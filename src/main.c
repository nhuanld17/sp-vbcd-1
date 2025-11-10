/* =============================================================================
 * MAIN.C - Deadlock Detection System Entry Point
 * =============================================================================
 * Main program that orchestrates deadlock detection with command-line interface,
 * signal handling, and resource management.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "config.h"
#include "utility.h"
#include "process_monitor.h"
#include "resource_graph.h"
#include "deadlock_detection.h"
#include "output_handler.h"
#include "email_alert.h"

/* =============================================================================
 * GLOBAL VARIABLES
 * =============================================================================
 */

static volatile int g_running = 1;  /* Signal flag for graceful shutdown */

/* =============================================================================
 * DATA STRUCTURES
 * =============================================================================
 */

typedef struct {
    int verbose;                    /* Verbose output flag */
    int continuous_monitor;          /* Continuous monitoring flag */
    int interval;                    /* Monitoring interval in seconds */
    char output_format[32];          /* Output format: text, json, verbose */
    char output_file[256];           /* Output file path (empty if stdout) */
    int alert_email;                 /* Email alert enabled flag */
    char email_recipients[MAX_EMAIL_RECIPIENTS_LEN];
    char log_file[MAX_PATH_LEN];
    char sender_name[MAX_SENDER_NAME_LEN];
    char smtp_server[256];
    int smtp_port;
    char from_email[MAX_EMAIL_RECIPIENTS_LEN];
} CommandLineArgs;

/* =============================================================================
 * SIGNAL HANDLING
 * =============================================================================
 */

/*
 * signal_handler - Handle SIGINT (Ctrl+C) for graceful shutdown
 * @sig: Signal number
 * @return: None
 */
static void signal_handler(int sig)
{
    (void)sig; /* Suppress unused parameter warning */
    g_running = 0;
    printf("\nReceived interrupt signal. Shutting down gracefully...\n");
}

/*
 * setup_signal_handlers - Register signal handlers
 * @return: SUCCESS (0) on success, negative on error
 */
static int setup_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        error_log("Failed to register SIGINT handler: %s", strerror(errno));
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    return SUCCESS;
}

static void apply_email_configuration(CommandLineArgs* args)
{
    if (args == NULL) {
        return;
    }

    char config_path[MAX_PATH_LEN];
    EmailConfig config;
    int result = ERROR_FILE_NOT_FOUND;
    
    /* Try current working directory first (project directory) */
    strncpy(config_path, "email.conf", sizeof(config_path) - 1);
    config_path[sizeof(config_path) - 1] = '\0';
    result = read_email_config(config_path, &config);
    
    /* If file not found in current directory, try home directory as fallback */
    if (result != SUCCESS) {
        const char* home_dir = getenv("HOME");
        if (home_dir == NULL || home_dir[0] == '\0') {
            home_dir = getenv("USERPROFILE"); /* Windows fallback */
        }
        
        if (home_dir != NULL && home_dir[0] != '\0') {
            snprintf(config_path, sizeof(config_path), "%s/.deadlock_detector/email.conf", home_dir);
            result = read_email_config(config_path, &config);
        }
    }
    
    if (result != SUCCESS) {
        return;
    }

    if (args->email_recipients[0] == '\0' && config.email_to[0] != '\0') {
        strncpy(args->email_recipients, config.email_to, sizeof(args->email_recipients) - 1);
        args->email_recipients[sizeof(args->email_recipients) - 1] = '\0';
    }

    if (args->sender_name[0] == '\0' && config.sender_name[0] != '\0') {
        strncpy(args->sender_name, config.sender_name, sizeof(args->sender_name) - 1);
        args->sender_name[sizeof(args->sender_name) - 1] = '\0';
    }

    if (args->smtp_server[0] == '\0' && config.smtp_server[0] != '\0') {
        strncpy(args->smtp_server, config.smtp_server, sizeof(args->smtp_server) - 1);
        args->smtp_server[sizeof(args->smtp_server) - 1] = '\0';
    }

    if (args->smtp_port == 0 && config.smtp_port > 0) {
        args->smtp_port = config.smtp_port;
    }

    if (args->from_email[0] == '\0' && config.from_email[0] != '\0') {
        strncpy(args->from_email, config.from_email, sizeof(args->from_email) - 1);
        args->from_email[sizeof(args->from_email) - 1] = '\0';
    }
}

/* =============================================================================
 * COMMAND-LINE ARGUMENT PARSING
 * =============================================================================
 */

/*
 * print_usage - Print program usage information
 * @program_name: Name of the program
 * @return: None
 */
static void print_usage(const char* program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Deadlock Detection System for Linux\n");
    printf("Monitors system processes and detects deadlock conditions.\n\n");
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --verbose           Enable verbose output\n");
    printf("  -c, --continuous        Continuous monitoring mode\n");
    printf("  -i, --interval SEC      Monitoring interval in seconds (default: %d)\n",
           DEFAULT_MONITORING_INTERVAL);
    printf("  -f, --format FORMAT     Output format: text, json, verbose (default: text)\n");
    printf("  -o, --output FILE       Write output to file instead of stdout\n");
    printf("      --alert TYPE        Alert mechanism (email or none)\n");
    printf("      --email-to LIST     Comma-separated email recipients for alerts\n");
    printf("      --log-file FILE     Append detection results to specified log file\n");
    printf("      --smtp-server HOST  SMTP server hostname (e.g., smtp.gmail.com)\n");
    printf("      --smtp-port PORT    SMTP server port (e.g., 25, 587)\n");
    printf("      --from-email EMAIL  Sender email address\n");
    printf("  --version               Show version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -v                    # One-time detection with verbose output\n", program_name);
    printf("  %s -c -i 10               # Continuous monitoring every 10 seconds\n", program_name);
    printf("  %s -f json -o report.json # JSON output to file\n", program_name);
}

/*
 * print_version - Print version information
 * @return: None
 */
static void print_version(void)
{
    printf("Deadlock Detector v%s\n", VERSION_STRING);
    printf("Copyright (C) 2024\n");
}

/*
 * parse_arguments - Parse command-line arguments
 * @argc: Argument count
 * @argv: Argument vector
 * @args: Output structure for parsed arguments
 * @return: SUCCESS (0) on success, negative on error, -1 for help, -2 for version
 * Description: Parses command-line arguments and validates them.
 * Error handling: Returns error codes for invalid arguments
 */
static int parse_arguments(int argc, char** argv, CommandLineArgs* args)
{
    if (args == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Initialize defaults */
    args->verbose = 0;
    args->continuous_monitor = 0;
    args->interval = DEFAULT_MONITORING_INTERVAL;
    strncpy(args->output_format, "text", sizeof(args->output_format) - 1);
    args->output_format[sizeof(args->output_format) - 1] = '\0';
    strncpy(args->output_file, "", sizeof(args->output_file) - 1);
    args->output_file[sizeof(args->output_file) - 1] = '\0';
    args->alert_email = 0;
    args->email_recipients[0] = '\0';
    args->log_file[0] = '\0';
    args->sender_name[0] = '\0';
    args->smtp_server[0] = '\0';
    args->smtp_port = 0;
    args->from_email[0] = '\0';
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return -1; /* Signal to print usage */
        }
        else if (strcmp(argv[i], "--version") == 0) {
            return -2; /* Signal to print version */
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            args->verbose = 1;
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--continuous") == 0) {
            args->continuous_monitor = 1;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -i/--interval requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            int interval = atoi(argv[++i]);
            if (interval < MIN_MONITORING_INTERVAL || interval > MAX_MONITORING_INTERVAL) {
                fprintf(stderr, "Error: interval must be between %d and %d seconds\n",
                       MIN_MONITORING_INTERVAL, MAX_MONITORING_INTERVAL);
                return ERROR_INVALID_ARGUMENT;
            }
            args->interval = interval;
        }
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f/--format requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            strncpy(args->output_format, argv[++i], sizeof(args->output_format) - 1);
            args->output_format[sizeof(args->output_format) - 1] = '\0';
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o/--output requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            strncpy(args->output_file, argv[++i], sizeof(args->output_file) - 1);
            args->output_file[sizeof(args->output_file) - 1] = '\0';
        }
        else if (strcmp(argv[i], "--alert") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --alert requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            const char* alert_mode = argv[++i];
            if (strcmp(alert_mode, "email") == 0) {
                args->alert_email = 1;
            } else if (strcmp(alert_mode, "none") == 0) {
                args->alert_email = 0;
            } else {
                fprintf(stderr, "Error: Unsupported alert mode '%s'\n", alert_mode);
                return ERROR_INVALID_ARGUMENT;
            }
        }
        else if (strcmp(argv[i], "--email-to") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --email-to requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            strncpy(args->email_recipients, argv[++i], sizeof(args->email_recipients) - 1);
            args->email_recipients[sizeof(args->email_recipients) - 1] = '\0';
        }
        else if (strcmp(argv[i], "--log-file") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --log-file requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            strncpy(args->log_file, argv[++i], sizeof(args->log_file) - 1);
            args->log_file[sizeof(args->log_file) - 1] = '\0';
        }
        else if (strcmp(argv[i], "--smtp-server") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --smtp-server requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            strncpy(args->smtp_server, argv[++i], sizeof(args->smtp_server) - 1);
            args->smtp_server[sizeof(args->smtp_server) - 1] = '\0';
        }
        else if (strcmp(argv[i], "--smtp-port") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --smtp-port requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            int port = atoi(argv[++i]);
            if (port <= 0 || port > 65535) {
                fprintf(stderr, "Error: SMTP port must be between 1 and 65535\n");
                return ERROR_INVALID_ARGUMENT;
            }
            args->smtp_port = port;
        }
        else if (strcmp(argv[i], "--from-email") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --from-email requires an argument\n");
                return ERROR_INVALID_ARGUMENT;
            }
            strncpy(args->from_email, argv[++i], sizeof(args->from_email) - 1);
            args->from_email[sizeof(args->from_email) - 1] = '\0';
        }
        else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            fprintf(stderr, "Use -h or --help for usage information\n");
            return ERROR_INVALID_ARGUMENT;
        }
    }
    
    return SUCCESS;
}

/* =============================================================================
 * MAIN DETECTION FUNCTION
 * =============================================================================
 */

/*
 * run_detection - Run one deadlock detection cycle
 * @args: Command-line arguments
 * @return: SUCCESS (0) on success, negative on error
 * Description: Performs one complete deadlock detection cycle:
 *              1. Collect process information
 *              2. Build Resource Allocation Graph
 *              3. Detect cycles
 *              4. Analyze and report deadlocks
 * Note: All allocated resources are properly freed, including DeadlockReport
 *       structure itself, even on error paths.
 */
static int run_detection(const CommandLineArgs* args)
{
    if (args == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Initialize all pointers to NULL for proper cleanup */
    pid_t* pids = NULL;
    ProcessResourceInfo* procs = NULL;
    DeadlockReport* report = NULL;
    int success_count = 0;
    int return_code = SUCCESS;
    
    /* Step 1: Collect process information */
    int num_procs = 0;
    pids = get_all_processes(&num_procs);
    
    if (pids == NULL) {
        error_log("Failed to get process list");
        return_code = ERROR_SYSTEM_CALL_FAILED;
        goto cleanup;
    }
    
    if (num_procs == 0) {
        info_log("No processes found");
        return_code = SUCCESS;
        goto cleanup;
    }
    
    if (args->verbose) {
        info_log("Collected %d processes", num_procs);
    }
    
    /* Step 2: Get process resource information */
    procs = (ProcessResourceInfo*)safe_malloc(
        sizeof(ProcessResourceInfo) * num_procs);
    if (procs == NULL) {
        return_code = ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    
    /* Initialize and collect resource info for each process */
    for (int i = 0; i < num_procs; i++) {
        memset(&procs[success_count], 0, sizeof(ProcessResourceInfo));
        int result = get_process_resources(pids[i], &procs[success_count]);
        if (result == SUCCESS) {
            success_count++;
        } else if (args->verbose) {
            debug_log("Failed to get resources for PID %d: %d", (int)pids[i], result);
        }
    }
    
    if (success_count == 0) {
        info_log("No process resource information available");
        return_code = SUCCESS;
        goto cleanup;
    }
    
    if (args->verbose) {
        info_log("Collected resource info for %d processes", success_count);
    }
    
    /* Step 2.5: Analyze pipe and lock dependencies */
    int dep_result = analyze_pipe_and_lock_dependencies(procs, success_count);
    if (dep_result != SUCCESS && args->verbose) {
        debug_log("Warning: Failed to analyze dependencies: %d", dep_result);
        /* Continue anyway, partial analysis may still work */
    } else if (args->verbose) {
        info_log("Analyzed pipe and lock dependencies");
    }
    
    /* Step 3: Detect deadlock */
    report = create_deadlock_report();
    if (report == NULL) {
        return_code = ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    
    int deadlock_status = detect_deadlock_in_system(procs, success_count, report);
    
    if (deadlock_status < 0) {
        error_log("Deadlock detection failed: %d", deadlock_status);
        return_code = deadlock_status;
        goto cleanup;
    }
    
    /* Step 4: Display results */
    if (deadlock_status > 0) {
        if (args->verbose) {
            info_log("DEADLOCK DETECTED!");
        }
        
        /* Parse output format */
        OutputFormat fmt = FORMAT_TEXT;
        int parse_result = parse_output_format(args->output_format, &fmt);
        if (parse_result != SUCCESS) {
            error_log("Invalid output format: %s, using default (text)", 
                     args->output_format);
            fmt = FORMAT_TEXT;
        }
        
        /* Display or export report */
        if (strlen(args->output_file) > 0) {
            int export_result = export_to_file(report, args->output_file, fmt);
            if (export_result != SUCCESS) {
                error_log("Failed to export report to file: %s", args->output_file);
                /* Non-fatal error, continue */
            }
        } else {
            int display_result = display_deadlock_report(report, fmt);
            if (display_result != SUCCESS) {
                error_log("Failed to display report: %d", display_result);
                /* Non-fatal error, continue */
            }
        }
    } else {
        if (args->verbose) {
            info_log("No deadlock detected");
        } else if (!args->continuous_monitor) {
            printf("No deadlock detected.\n");
        }
    }
    
    return_code = SUCCESS;
    
cleanup:
    /* Step 5: Cleanup - ensure all resources are freed */
    /* Free DeadlockReport (frees structure and all nested allocations) */
    if (report != NULL) {
        free_deadlock_report(report);
        report = NULL;
    }
    
    /* Free process resource info */
    if (procs != NULL) {
        for (int i = 0; i < success_count; i++) {
            free_process_resource_info(&procs[i]);
        }
        free(procs);
        procs = NULL;
    }
    
    /* Free process list */
    if (pids != NULL) {
        free_process_list(pids);
        pids = NULL;
    }
    
    return return_code;
}

/* =============================================================================
 * MAIN FUNCTION
 * =============================================================================
 */

int main(int argc, char** argv)
{
    CommandLineArgs args;
    int parse_result = parse_arguments(argc, argv, &args);
    
    /* Handle special return codes */
    if (parse_result == -1) {
        print_usage(argv[0]);
        return 0;
    } else if (parse_result == -2) {
        print_version();
        return 0;
    } else if (parse_result < 0) {
        return 1;
    }

    apply_email_configuration(&args);

    EmailAlertOptions alert_options;
    memset(&alert_options, 0, sizeof(alert_options));
    alert_options.enable_email = args.alert_email;
    strncpy(alert_options.recipients, args.email_recipients,
            sizeof(alert_options.recipients) - 1);
    alert_options.recipients[sizeof(alert_options.recipients) - 1] = '\0';

    strncpy(alert_options.log_file, args.log_file,
            sizeof(alert_options.log_file) - 1);
    alert_options.log_file[sizeof(alert_options.log_file) - 1] = '\0';

    strncpy(alert_options.sender_name, args.sender_name,
            sizeof(alert_options.sender_name) - 1);
    alert_options.sender_name[sizeof(alert_options.sender_name) - 1] = '\0';

    strncpy(alert_options.smtp_server, args.smtp_server,
            sizeof(alert_options.smtp_server) - 1);
    alert_options.smtp_server[sizeof(alert_options.smtp_server) - 1] = '\0';

    alert_options.smtp_port = args.smtp_port;

    strncpy(alert_options.from_email, args.from_email,
            sizeof(alert_options.from_email) - 1);
    alert_options.from_email[sizeof(alert_options.from_email) - 1] = '\0';

    email_alert_set_options(&alert_options);
    
    /* Setup signal handlers for graceful shutdown */
    if (setup_signal_handlers() != SUCCESS) {
        error_log("Failed to setup signal handlers");
        return 1;
    }
    
    /* Print startup information */
    if (args.verbose) {
        info_log("Deadlock Detection System Started");
        info_log("Version: %s", VERSION_STRING);
        info_log("Format: %s", args.output_format);
        info_log("Continuous: %s", args.continuous_monitor ? "yes" : "no");
        if (args.continuous_monitor) {
            info_log("Interval: %d seconds", args.interval);
        }
        if (strlen(args.output_file) > 0) {
            info_log("Output file: %s", args.output_file);
        }
        printf("\n");
    }
    
    /* Main detection loop */
    int result = SUCCESS;
    do {
        /* Check if we should continue running */
        if (!g_running) {
            if (args.verbose) {
                info_log("Shutdown requested, exiting...");
            }
            break;
        }
        
        /* Run detection */
        result = run_detection(&args);
        
        if (result != SUCCESS) {
            error_log("Detection cycle failed: %d", result);
            if (!args.continuous_monitor) {
                break; /* Exit on error in one-shot mode */
            }
        }
        
        /* Wait before next cycle if continuous */
        if (args.continuous_monitor && g_running) {
            /* Sleep in small intervals to check g_running flag */
            for (int i = 0; i < args.interval && g_running; i++) {
                sleep(1);
            }
        }
        
    } while (args.continuous_monitor && g_running);
    
    if (args.verbose) {
        info_log("Deadlock Detection System Stopped");
    }
    
    return (result == SUCCESS) ? 0 : 1;
}

