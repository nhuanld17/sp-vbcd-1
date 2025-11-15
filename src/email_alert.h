#ifndef EMAIL_ALERT_H
#define EMAIL_ALERT_H

#include <stddef.h>
#include "config.h"
#include "deadlock_detection.h"
#include "smtp_client.h"

typedef struct {
    char email_to[MAX_EMAIL_RECIPIENTS_LEN];
    char sender_name[MAX_SENDER_NAME_LEN];
    char smtp_server[256];
    int smtp_port;
    char from_email[MAX_EMAIL_RECIPIENTS_LEN];
} EmailConfig;

typedef struct {
    int enable_email;
    char recipients[MAX_EMAIL_RECIPIENTS_LEN];
    char log_file[MAX_PATH_LEN];
    char sender_name[MAX_SENDER_NAME_LEN];
    char smtp_server[256];
    int smtp_port;
    char from_email[MAX_EMAIL_RECIPIENTS_LEN];
} EmailAlertOptions;

typedef struct {
    int total_recipients;
    int successful_recipients;
} EmailSendResult;

int read_email_config(const char *config_file, EmailConfig *config);
int write_log_file(const char *log_path, const char *message);
int send_email_alert(const char *email_to, const char *subject, const char *body);
void email_alert_get_last_result(EmailSendResult *result);
void email_alert_get_last_status(char *buffer, size_t size);
char *build_deadlock_email_body(const DeadlockReport *report, const char *sender_name);
void email_alert_set_options(const EmailAlertOptions *options);
void email_alert_handle_detection(const DeadlockReport *report, int deadlock_status);

#endif /* EMAIL_ALERT_H */


