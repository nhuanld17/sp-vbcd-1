#include "email_alert.h"
#include "utility.h"
#include "process_monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdint.h>

static EmailSendResult g_last_result = {0, 0};
static char g_last_status[512] = {0};
static EmailAlertOptions g_alert_options;

static void reset_last_status(void);
static void format_current_timestamp(char *buffer, size_t size);
static void build_deadlocked_process_log(const DeadlockReport *report, char *buffer, size_t size);

static void reset_last_status(void)
{
    g_last_result.total_recipients = 0;
    g_last_result.successful_recipients = 0;
    g_last_status[0] = '\0';
}

static void format_current_timestamp(char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return;
    }

    time_t now = time(NULL);
    struct tm tm_now;
    if (localtime_r(&now, &tm_now) == NULL) {
        buffer[0] = '\0';
        return;
    }

    if (strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm_now) == 0) {
        buffer[0] = '\0';
    }
}

static void build_deadlocked_process_log(const DeadlockReport *report, char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return;
    }

    buffer[0] = '\0';

    if (report == NULL || report->deadlocked_pids == NULL || report->num_deadlocked == 0) {
        snprintf(buffer, size, "None");
        return;
    }

    size_t offset = 0;
    for (int i = 0; i < report->num_deadlocked; i++) {
        int pid = report->deadlocked_pids[i];
        ProcessInfo info;
        memset(&info, 0, sizeof(ProcessInfo));
        char proc_name[MAX_PROCESS_NAME_LEN];
        proc_name[0] = '\0';

        if (get_process_info((pid_t)pid, &info) == SUCCESS) {
            strncpy(proc_name, info.name, sizeof(proc_name) - 1);
            proc_name[sizeof(proc_name) - 1] = '\0';
            free_process_info(&info);
        } else {
            snprintf(proc_name, sizeof(proc_name), "Unknown");
        }

        int written = snprintf(buffer + offset, size - offset, "%sPID %d (%s)",
                               (offset > 0) ? ", " : "", pid, proc_name);
        if (written < 0 || (size_t)written >= size - offset) {
            buffer[size - 1] = '\0';
            break;
        }
        offset += (size_t)written;
    }
}

void email_alert_get_last_result(EmailSendResult *result)
{
    if (result == NULL) {
        return;
    }
    result->total_recipients = g_last_result.total_recipients;
    result->successful_recipients = g_last_result.successful_recipients;
}

void email_alert_get_last_status(char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return;
    }
    if (g_last_status[0] == '\0') {
        buffer[0] = '\0';
        return;
    }
    strncpy(buffer, g_last_status, size - 1);
    buffer[size - 1] = '\0';
}

int read_email_config(const char *config_file, EmailConfig *config)
{
    if (config == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }

    config->email_to[0] = '\0';
    config->sender_name[0] = '\0';
    config->smtp_server[0] = '\0';
    config->smtp_port = 0;
    config->from_email[0] = '\0';

    if (config_file == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }

    FILE *fp = fopen(config_file, "r");
    if (fp == NULL) {
        return ERROR_FILE_NOT_FOUND;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *trimmed = str_trim(line);
        if (trimmed == NULL || trimmed[0] == '\0') {
            continue;
        }
        if (trimmed[0] == '#') {
            continue;
        }

        char *separator = strchr(trimmed, '=');
        if (separator == NULL) {
            continue;
        }

        *separator = '\0';
        separator++;

        char *key = str_trim(trimmed);
        char *value = str_trim(separator);
        if (key == NULL || value == NULL) {
            continue;
        }

        if (strcmp(key, "email_to") == 0) {
            strncpy(config->email_to, value, sizeof(config->email_to) - 1);
            config->email_to[sizeof(config->email_to) - 1] = '\0';
        } else if (strcmp(key, "sender_name") == 0) {
            strncpy(config->sender_name, value, sizeof(config->sender_name) - 1);
            config->sender_name[sizeof(config->sender_name) - 1] = '\0';
        } else if (strcmp(key, "smtp_server") == 0) {
            strncpy(config->smtp_server, value, sizeof(config->smtp_server) - 1);
            config->smtp_server[sizeof(config->smtp_server) - 1] = '\0';
        } else if (strcmp(key, "smtp_port") == 0) {
            config->smtp_port = atoi(value);
        } else if (strcmp(key, "from_email") == 0) {
            strncpy(config->from_email, value, sizeof(config->from_email) - 1);
            config->from_email[sizeof(config->from_email) - 1] = '\0';
        }
    }

    fclose(fp);
    return SUCCESS;
}

int write_log_file(const char *log_path, const char *message)
{
    if (log_path == NULL || message == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }

    FILE *fp = fopen(log_path, "a");
    if (fp == NULL) {
        return ERROR_SYSTEM_CALL_FAILED;
    }

    if (fputs(message, fp) < 0) {
        fclose(fp);
        return ERROR_SYSTEM_CALL_FAILED;
    }

    /* Ensure newline termination */
    size_t len = strlen(message);
    if (len == 0 || message[len - 1] != '\n') {
        if (fputc('\n', fp) == EOF) {
            fclose(fp);
            return ERROR_SYSTEM_CALL_FAILED;
        }
    }

    fclose(fp);
    return SUCCESS;
}

static void escape_quotes(const char *src, char *dst, size_t dst_size)
{
    if (dst_size == 0) {
        return;
    }

    size_t pos = 0;
    for (size_t i = 0; src != NULL && src[i] != '\0'; i++) {
        if (src[i] == '"' || src[i] == '\\') {
            if (pos + 2 >= dst_size) {
                break;
            }
            dst[pos++] = '\\';
            dst[pos++] = src[i];
        } else {
            if (pos + 1 >= dst_size) {
                break;
            }
            dst[pos++] = src[i];
        }
    }
    dst[pos] = '\0';
}

static int append_status(const char *recipient, int success)
{
    char segment[128];
    snprintf(segment, sizeof(segment), "%s=%s",
             recipient != NULL ? recipient : "(null)",
             success ? "SUCCESS" : "FAILED");

    size_t current_len = strlen(g_last_status);
    size_t segment_len = strlen(segment);
    if (current_len > 0) {
        if (current_len + 2 < sizeof(g_last_status)) {
            strncat(g_last_status, ", ", sizeof(g_last_status) - current_len - 1);
        } else {
            return ERROR_BUFFER_OVERFLOW;
        }
    }

    current_len = strlen(g_last_status);
    if (current_len + segment_len < sizeof(g_last_status)) {
        strncat(g_last_status, segment, sizeof(g_last_status) - current_len - 1);
        return SUCCESS;
    }

    return ERROR_BUFFER_OVERFLOW;
}

int send_email_alert(const char *email_to, const char *subject, const char *body)
{
    fprintf(stderr, "[EMAIL] send_email_alert() called\n");
    fprintf(stderr, "[EMAIL] Recipient: '%s'\n", email_to ? email_to : "(NULL)");
    fprintf(stderr, "[EMAIL] Subject: '%s'\n", subject ? subject : "(NULL)");
    
    reset_last_status();

    if (email_to == NULL || subject == NULL || body == NULL) {
        fprintf(stderr, "[EMAIL] ERROR: Invalid arguments (NULL)\n");
        return ERROR_INVALID_ARGUMENT;
    }

    if (email_to[0] == '\0') {
        fprintf(stderr, "[EMAIL] ERROR: Empty email_to\n");
        return ERROR_INVALID_ARGUMENT;
    }

    /* Use mail command (works with ssmtp/sendmail) */
    fprintf(stderr, "[EMAIL] Using mail command\n");
    char recipients_copy[MAX_EMAIL_RECIPIENTS_LEN];
    strncpy(recipients_copy, email_to, sizeof(recipients_copy) - 1);
    recipients_copy[sizeof(recipients_copy) - 1] = '\0';

    char temp_file[256];
    snprintf(temp_file, sizeof(temp_file), "/tmp/deadlock_email_%d_%ld.txt",
             (int)getpid(), (long)time(NULL));
    fprintf(stderr, "[EMAIL] Temp file: %s\n", temp_file);

    FILE *fp = fopen(temp_file, "w");
    if (fp == NULL) {
        fprintf(stderr, "[EMAIL] ERROR: Failed to create temp file\n");
        return ERROR_SYSTEM_CALL_FAILED;
    }

    if (fprintf(fp, "%s", body) < 0) {
        fprintf(stderr, "[EMAIL] ERROR: Failed to write to temp file\n");
        fclose(fp);
        unlink(temp_file);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    fclose(fp);
    fprintf(stderr, "[EMAIL] Temp file written successfully\n");

    char subject_escaped[MAX_EMAIL_SUBJECT_LEN * 4];
    escape_quotes(subject, subject_escaped, sizeof(subject_escaped));

    char *saveptr = NULL;
    char *token = strtok_r(recipients_copy, ",", &saveptr);
    int last_error = SUCCESS;

    while (token != NULL) {
        char *recipient = str_trim(token);
        if (recipient != NULL && recipient[0] != '\0') {
            g_last_result.total_recipients++;

            char command[1024];
            snprintf(command, sizeof(command),
                     "mail -s \"%s\" \"%s\" < %s",
                     subject_escaped,
                     recipient,
                     temp_file);
            fprintf(stderr, "[EMAIL] Executing command: %s\n", command);

            int sys_result = system(command);
            int success = 0;
            if (sys_result == -1) {
                fprintf(stderr, "[EMAIL] ERROR: system() call failed\n");
                last_error = ERROR_SYSTEM_CALL_FAILED;
            } else if (WIFEXITED(sys_result)) {
                int exit_code = WEXITSTATUS(sys_result);
                fprintf(stderr, "[EMAIL] mail command exit code: %d\n", exit_code);
                if (exit_code == 0) {
                    success = 1;
                    g_last_result.successful_recipients++;
                    fprintf(stderr, "[EMAIL] SUCCESS: Email sent to %s\n", recipient);
                } else {
                    if (exit_code == 127) {
                        fprintf(stderr, "[EMAIL] ERROR: mail command not found (exit 127)\n");
                        last_error = ERROR_SYSTEM_CALL_FAILED;
                    } else {
                        fprintf(stderr, "[EMAIL] ERROR: mail command failed (exit %d)\n", exit_code);
                        last_error = ERROR_SYSTEM_CALL_FAILED;
                    }
                }
            } else {
                fprintf(stderr, "[EMAIL] ERROR: mail command did not exit normally\n");
                last_error = ERROR_SYSTEM_CALL_FAILED;
            }

            append_status(recipient, success);
        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    unlink(temp_file);

    if (g_last_result.total_recipients == 0) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (g_last_result.successful_recipients == g_last_result.total_recipients) {
        return SUCCESS;
    }

    if (g_last_result.successful_recipients > 0) {
        return 1;
    }

    if (last_error != SUCCESS) {
        return last_error;
    }

    return ERROR_SYSTEM_CALL_FAILED;
}

static int ensure_capacity(char **buffer, size_t *capacity, size_t required)
{
    if (*buffer == NULL || capacity == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (required <= *capacity) {
        return SUCCESS;
    }

    size_t new_capacity = *capacity;
    while (new_capacity < required) {
        new_capacity *= 2;
        if (new_capacity < required) {
            if (new_capacity > (SIZE_MAX / 2)) {
                return ERROR_OUT_OF_MEMORY;
            }
        }
    }

    char *new_buffer = safe_realloc(*buffer, new_capacity);
    if (new_buffer == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }

    *buffer = new_buffer;
    *capacity = new_capacity;
    return SUCCESS;
}

static int append_text(char **buffer, size_t *capacity, size_t *length, const char *text)
{
    if (buffer == NULL || capacity == NULL || length == NULL || text == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }

    size_t text_len = strlen(text);
    size_t required = *length + text_len + 1;

    int result = ensure_capacity(buffer, capacity, required);
    if (result != SUCCESS) {
        return result;
    }

    memcpy(*buffer + *length, text, text_len);
    *length += text_len;
    (*buffer)[*length] = '\0';

    return SUCCESS;
}

char *build_deadlock_email_body(const DeadlockReport *report, const char *sender_name)
{
    if (report == NULL) {
        return NULL;
    }

    size_t capacity = 4096;
    char *buffer = safe_malloc(capacity);
    if (buffer == NULL) {
        return NULL;
    }
    buffer[0] = '\0';
    size_t length = 0;

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_now);

    append_text(&buffer, &capacity, &length, "Deadlock Alert Notification\n");
    append_text(&buffer, &capacity, &length, "========================================\n");

    char line[256];
    snprintf(line, sizeof(line), "Timestamp: %s\n", timestamp);
    append_text(&buffer, &capacity, &length, line);

    snprintf(line, sizeof(line), "Deadlock Detected: %s\n", report->deadlock_detected ? "YES" : "NO");
    append_text(&buffer, &capacity, &length, line);

    snprintf(line, sizeof(line), "Number of Cycles Detected: %d\n", report->num_cycles);
    append_text(&buffer, &capacity, &length, line);

    snprintf(line, sizeof(line), "Deadlocked Processes: %d\n", report->num_deadlocked);
    append_text(&buffer, &capacity, &length, line);

    snprintf(line, sizeof(line), "Processes Scanned: %d\n", report->total_processes_scanned);
    append_text(&buffer, &capacity, &length, line);

    snprintf(line, sizeof(line), "Resources Identified: %d\n\n", report->total_resources_found);
    append_text(&buffer, &capacity, &length, line);

    if (report->deadlock_detected && report->deadlocked_pids != NULL && report->num_deadlocked > 0) {
        append_text(&buffer, &capacity, &length, "Deadlocked Process Details:\n");
        for (int i = 0; i < report->num_deadlocked; i++) {
            int pid = report->deadlocked_pids[i];
            ProcessInfo info;
            memset(&info, 0, sizeof(ProcessInfo));
            char name_buffer[MAX_PROCESS_NAME_LEN];
            name_buffer[0] = '\0';

            if (get_process_info((pid_t)pid, &info) == SUCCESS) {
                strncpy(name_buffer, info.name, sizeof(name_buffer) - 1);
                name_buffer[sizeof(name_buffer) - 1] = '\0';
                free_process_info(&info);
            } else {
                snprintf(name_buffer, sizeof(name_buffer), "Unknown");
            }

            snprintf(line, sizeof(line), "  - PID %d : %s\n", pid, name_buffer);
            append_text(&buffer, &capacity, &length, line);
        }
        append_text(&buffer, &capacity, &length, "\n");
    }

    if (report->num_cycles > 0 && report->cycles != NULL) {
        append_text(&buffer, &capacity, &length, "Cycle Summary:\n");
        for (int i = 0; i < report->num_cycles; i++) {
            snprintf(line, sizeof(line), "  * Cycle #%d involves %d processes and %d resources\n",
                     i + 1,
                     report->cycles[i].num_processes,
                     report->cycles[i].num_resources);
            append_text(&buffer, &capacity, &length, line);
        }
        append_text(&buffer, &capacity, &length, "\n");
    }

    if (report->num_recommendations > 0 && report->recommendations != NULL) {
        append_text(&buffer, &capacity, &length, "Recommended Actions:\n");
        for (int i = 0; i < report->num_recommendations; i++) {
            snprintf(line, sizeof(line), "  - %s\n", report->recommendations[i]);
            append_text(&buffer, &capacity, &length, line);
        }
        append_text(&buffer, &capacity, &length, "\n");
    }

    append_text(&buffer, &capacity, &length, "Additional Notes:\n");
    append_text(&buffer, &capacity, &length, "  • Review resource allocation policies.\n");
    append_text(&buffer, &capacity, &length, "  • Consider terminating a deadlocked process to resolve the issue.\n");
    append_text(&buffer, &capacity, &length, "  • Implement safeguards to prevent future deadlocks.\n\n");

    if (sender_name != NULL && sender_name[0] != '\0') {
        snprintf(line, sizeof(line), "Alert Triggered By: %s\n", sender_name);
        append_text(&buffer, &capacity, &length, line);
    }

    append_text(&buffer, &capacity, &length, "========================================\n");
    append_text(&buffer, &capacity, &length, "Deadlock Detector Automated Alert System\n");

    return buffer;
}

void email_alert_set_options(const EmailAlertOptions *options)
{
    if (options == NULL) {
        memset(&g_alert_options, 0, sizeof(g_alert_options));
        return;
    }

    g_alert_options.enable_email = options->enable_email;
    strncpy(g_alert_options.recipients, options->recipients,
            sizeof(g_alert_options.recipients) - 1);
    g_alert_options.recipients[sizeof(g_alert_options.recipients) - 1] = '\0';

    strncpy(g_alert_options.log_file, options->log_file,
            sizeof(g_alert_options.log_file) - 1);
    g_alert_options.log_file[sizeof(g_alert_options.log_file) - 1] = '\0';

    strncpy(g_alert_options.sender_name, options->sender_name,
            sizeof(g_alert_options.sender_name) - 1);
    g_alert_options.sender_name[sizeof(g_alert_options.sender_name) - 1] = '\0';

    strncpy(g_alert_options.smtp_server, options->smtp_server,
            sizeof(g_alert_options.smtp_server) - 1);
    g_alert_options.smtp_server[sizeof(g_alert_options.smtp_server) - 1] = '\0';

    g_alert_options.smtp_port = options->smtp_port;

    strncpy(g_alert_options.from_email, options->from_email,
            sizeof(g_alert_options.from_email) - 1);
    g_alert_options.from_email[sizeof(g_alert_options.from_email) - 1] = '\0';
}

void email_alert_handle_detection(const DeadlockReport *report, int deadlock_status)
{
    fprintf(stderr, "[EMAIL] === EMAIL ALERT TRIGGERED ===\n");
    fprintf(stderr, "[EMAIL] deadlock_status: %d\n", deadlock_status);
    fprintf(stderr, "[EMAIL] enable_email: %d\n", g_alert_options.enable_email);
    fprintf(stderr, "[EMAIL] recipients: '%s'\n", g_alert_options.recipients);
    fprintf(stderr, "[EMAIL] smtp_server: '%s'\n", g_alert_options.smtp_server);
    fprintf(stderr, "[EMAIL] smtp_port: %d\n", g_alert_options.smtp_port);
    
    if (report == NULL) {
        fprintf(stderr, "[EMAIL] ERROR: report is NULL\n");
        return;
    }

    char timestamp[64];
    format_current_timestamp(timestamp, sizeof(timestamp));

    int email_attempted = 0;
    int email_send_code = 0;
    EmailSendResult email_result = {0, 0};
    char email_status_label[16] = "NOT_SENT";
    char email_status_summary[512];
    email_status_summary[0] = '\0';

    if (!g_alert_options.enable_email) {
        fprintf(stderr, "[EMAIL] Email alert is DISABLED\n");
        strncpy(email_status_label, "DISABLED", sizeof(email_status_label) - 1);
        email_status_label[sizeof(email_status_label) - 1] = '\0';
        snprintf(email_status_summary, sizeof(email_status_summary), "Email alert disabled");
    }

    if (deadlock_status > 0) {
        fprintf(stderr, "[EMAIL] Deadlock detected (status=%d), attempting to send email\n", deadlock_status);
        if (g_alert_options.enable_email) {
            if (g_alert_options.recipients[0] == '\0') {
                fprintf(stderr, "[EMAIL] ERROR: No recipients configured\n");
                strncpy(email_status_label, "NOT_SENT", sizeof(email_status_label) - 1);
                email_status_label[sizeof(email_status_label) - 1] = '\0';
                snprintf(email_status_summary, sizeof(email_status_summary),
                         "No recipients configured");
            } else {
                fprintf(stderr, "[EMAIL] Building email body...\n");
                char subject[MAX_EMAIL_SUBJECT_LEN];
                if (timestamp[0] != '\0') {
                    snprintf(subject, sizeof(subject), "DEADLOCK ALERT: %s", timestamp);
                } else {
                    strncpy(subject, "DEADLOCK ALERT", sizeof(subject) - 1);
                    subject[sizeof(subject) - 1] = '\0';
                }

                char *body = build_deadlock_email_body(report, g_alert_options.sender_name);
                if (body == NULL) {
                    fprintf(stderr, "[EMAIL] ERROR: Failed to build email body\n");
                    strncpy(email_status_label, "FAILED", sizeof(email_status_label) - 1);
                    email_status_label[sizeof(email_status_label) - 1] = '\0';
                    snprintf(email_status_summary, sizeof(email_status_summary),
                             "Failed to build email body");
                } else {
                    fprintf(stderr, "[EMAIL] Email body built successfully, calling send_email_alert()\n");
                    email_attempted = 1;
                    email_send_code = send_email_alert(g_alert_options.recipients, subject, body);
                    email_alert_get_last_result(&email_result);
                    email_alert_get_last_status(email_status_summary, sizeof(email_status_summary));
                    if (email_status_summary[0] == '\0') {
                        strncpy(email_status_summary, g_alert_options.recipients,
                                sizeof(email_status_summary) - 1);
                        email_status_summary[sizeof(email_status_summary) - 1] = '\0';
                    }

                    if (email_send_code == SUCCESS) {
                        strncpy(email_status_label, "SUCCESS", sizeof(email_status_label) - 1);
                    } else if (email_send_code == 1) {
                        strncpy(email_status_label, "PARTIAL", sizeof(email_status_label) - 1);
                    } else {
                        strncpy(email_status_label, "FAILED", sizeof(email_status_label) - 1);
                    }
                    email_status_label[sizeof(email_status_label) - 1] = '\0';

                    if (email_send_code != SUCCESS) {
                        error_log("Email alert returned status %d", email_send_code);
                    }

                    free(body);
                }
            }
        }
    } else {
        fprintf(stderr, "[EMAIL] No deadlock detected (status=%d), email NOT sent\n", deadlock_status);
        if (g_alert_options.enable_email) {
            strncpy(email_status_label, "NOT_TRIGGERED", sizeof(email_status_label) - 1);
            email_status_label[sizeof(email_status_label) - 1] = '\0';
            snprintf(email_status_summary, sizeof(email_status_summary),
                     "No deadlock detected");
        }
    }

    if (g_alert_options.log_file[0] != '\0') {
        char log_entry[4096];
        size_t offset = 0;
        const char *ts = (timestamp[0] != '\0') ? timestamp : "UNKNOWN";
        char process_line[1024];
        build_deadlocked_process_log(report, process_line, sizeof(process_line));

        char email_log_line[1024];
        if (!g_alert_options.enable_email) {
            snprintf(email_log_line, sizeof(email_log_line), "Email alert disabled");
        } else if (deadlock_status > 0) {
            const char *summary = (email_status_summary[0] != '\0') ?
                                  email_status_summary : "No details";
            if (email_attempted) {
                snprintf(email_log_line, sizeof(email_log_line),
                         "Email sent to: %s (%s %d/%d)",
                         summary,
                         email_status_label,
                         email_result.successful_recipients,
                         email_result.total_recipients);
            } else {
                snprintf(email_log_line, sizeof(email_log_line),
                         "Email not sent: %s (%s)",
                         summary,
                         email_status_label);
            }
        } else {
            const char *summary = (email_status_summary[0] != '\0') ?
                                  email_status_summary : "No deadlock detected";
            snprintf(email_log_line, sizeof(email_log_line),
                     "Email alert state: %s (%s)",
                     summary,
                     email_status_label);
        }

        offset += snprintf(log_entry + offset, sizeof(log_entry) - offset,
                           "[%s] %s\n",
                           ts,
                           deadlock_status > 0 ? "DEADLOCK DETECTED" : "No deadlock detected");

        if (deadlock_status > 0) {
            offset += snprintf(log_entry + offset, sizeof(log_entry) - offset,
                               "  Processes: %s\n", process_line);
            offset += snprintf(log_entry + offset, sizeof(log_entry) - offset,
                               "  Cycles: %d\n", report->num_cycles);
        }

        offset += snprintf(log_entry + offset, sizeof(log_entry) - offset,
                           "  Email: %s\n", email_log_line);

        int log_write_result = write_log_file(g_alert_options.log_file, log_entry);
        if (log_write_result != SUCCESS) {
            error_log("Failed to write log file '%s': %d",
                      g_alert_options.log_file, log_write_result);
        }
    }
}



