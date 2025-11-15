/* =============================================================================
 * SMTP_CLIENT.C - SMTP Client Implementation (Pure C)
 * =============================================================================
 * Direct SMTP protocol implementation using raw TCP sockets.
 * No external libraries - only standard C and POSIX socket API.
 * =============================================================================
 */

#include "smtp_client.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* =============================================================================
 * HELPER FUNCTIONS
 * =============================================================================
 */

/*
 * parse_smtp_response - Parse SMTP server response code
 * @response: SMTP server response string
 * @return: Response code (e.g., 220, 250) or -1 on parse error
 */
int parse_smtp_response(const char *response)
{
    if (response == NULL || strlen(response) < 3) {
        return -1;
    }
    
    /* SMTP responses start with 3-digit code */
    if (response[0] >= '0' && response[0] <= '9' &&
        response[1] >= '0' && response[1] <= '9' &&
        response[2] >= '0' && response[2] <= '9') {
        int code = (response[0] - '0') * 100 +
                   (response[1] - '0') * 10 +
                   (response[2] - '0');
        return code;
    }
    
    return -1;
}

/*
 * send_smtp_command - Send command to SMTP server and read response
 * @sock: Socket file descriptor
 * @command: SMTP command to send
 * @response: Buffer to store server response
 * @response_size: Size of response buffer
 * @return: Response code on success, -1 on error
 */
int send_smtp_command(int sock, const char *command,
                      char *response, size_t response_size)
{
    if (sock < 0 || command == NULL || response == NULL || response_size == 0) {
        return -1;
    }
    
    /* Send command */
    ssize_t sent = send(sock, command, strlen(command), 0);
    if (sent < 0) {
        error_log("Failed to send SMTP command: %s", strerror(errno));
        return -1;
    }
    
    /* Read response */
    ssize_t received = recv(sock, response, response_size - 1, 0);
    if (received < 0) {
        error_log("Failed to receive SMTP response: %s", strerror(errno));
        return -1;
    }
    
    if (received == 0) {
        error_log("SMTP server closed connection");
        return -1;
    }
    
    response[received] = '\0';
    
    /* Parse response code */
    int code = parse_smtp_response(response);
    if (code < 0) {
        error_log("Invalid SMTP response format: %s", response);
        return -1;
    }
    
    return code;
}

/*
 * format_email_date - Format current date for email Date header
 * @buffer: Output buffer
 * @size: Buffer size
 * @return: SUCCESS on success, negative on error
 */
static int format_email_date(char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if (tm_now == NULL) {
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Format: Mon, 01 Jan 2024 12:00:00 +0000 */
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    int written = snprintf(buffer, size,
                          "%s, %02d %s %04d %02d:%02d:%02d +0000",
                          days[tm_now->tm_wday],
                          tm_now->tm_mday,
                          months[tm_now->tm_mon],
                          tm_now->tm_year + 1900,
                          tm_now->tm_hour,
                          tm_now->tm_min,
                          tm_now->tm_sec);
    
    if (written < 0 || (size_t)written >= size) {
        return ERROR_BUFFER_OVERFLOW;
    }
    
    return SUCCESS;
}

/* =============================================================================
 * MAIN FUNCTION
 * =============================================================================
 */

/*
 * send_email_via_smtp - Send email via direct SMTP connection
 */
int send_email_via_smtp(const char *smtp_server, int smtp_port,
                        const char *from_email, const char *to_email,
                        const char *subject, const char *body)
{
    if (smtp_server == NULL || from_email == NULL || 
        to_email == NULL || subject == NULL || body == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (smtp_port <= 0 || smtp_port > 65535) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    int sock = -1;
    int result = ERROR_SYSTEM_CALL_FAILED;
    
    /* Step 1: Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        error_log("Failed to create socket: %s", strerror(errno));
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Step 2: Resolve hostname */
    struct hostent *server = gethostbyname(smtp_server);
    if (server == NULL) {
        error_log("Failed to resolve hostname: %s", smtp_server);
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Step 3: Setup server address */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(smtp_port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    /* Step 4: Connect to SMTP server */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (strcmp(smtp_server, "localhost") == 0 && smtp_port == 25) {
            error_log("Cannot connect to localhost:25. Install postfix: sudo apt-get install postfix -y");
            error_log("Or configure your local SMTP server to listen on port 25");
        } else {
            error_log("Failed to connect to SMTP server %s:%d: %s",
                     smtp_server, smtp_port, strerror(errno));
        }
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    char response[1024];
    int response_code;
    
    /* Step 5: Read server greeting (220) */
    ssize_t received = recv(sock, response, sizeof(response) - 1, 0);
    if (received < 0) {
        error_log("Failed to receive SMTP greeting: %s", strerror(errno));
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    response[received] = '\0';
    
    response_code = parse_smtp_response(response);
    if (response_code != 220) {
        error_log("SMTP server greeting failed: %s (code: %d)", response, response_code);
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Step 6: Send HELO */
    char helo_cmd[256];
    snprintf(helo_cmd, sizeof(helo_cmd), "HELO localhost\r\n");
    response_code = send_smtp_command(sock, helo_cmd, response, sizeof(response));
    if (response_code != 250) {
        error_log("SMTP HELO failed: %s (code: %d)", response, response_code);
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Step 7: Send MAIL FROM */
    char mail_from_cmd[512];
    snprintf(mail_from_cmd, sizeof(mail_from_cmd), "MAIL FROM:<%s>\r\n", from_email);
    response_code = send_smtp_command(sock, mail_from_cmd, response, sizeof(response));
    if (response_code != 250) {
        error_log("SMTP MAIL FROM failed: %s (code: %d)", response, response_code);
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Step 8: Send RCPT TO for each recipient */
    char recipients_copy[MAX_EMAIL_RECIPIENTS_LEN];
    strncpy(recipients_copy, to_email, sizeof(recipients_copy) - 1);
    recipients_copy[sizeof(recipients_copy) - 1] = '\0';
    
    char *saveptr = NULL;
    char *token = strtok_r(recipients_copy, ",", &saveptr);
    int recipients_sent = 0;
    
    while (token != NULL) {
        char *recipient = str_trim(token);
        if (recipient != NULL && recipient[0] != '\0') {
            char rcpt_cmd[512];
            snprintf(rcpt_cmd, sizeof(rcpt_cmd), "RCPT TO:<%s>\r\n", recipient);
            response_code = send_smtp_command(sock, rcpt_cmd, response, sizeof(response));
            if (response_code == 250 || response_code == 251) {
                recipients_sent++;
            } else {
                error_log("SMTP RCPT TO failed for %s: %s (code: %d)",
                         recipient, response, response_code);
            }
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
    
    if (recipients_sent == 0) {
        error_log("No valid recipients");
        close(sock);
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Step 9: Send DATA */
    response_code = send_smtp_command(sock, "DATA\r\n", response, sizeof(response));
    if (response_code != 354) {
        error_log("SMTP DATA command failed: %s (code: %d)", response, response_code);
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Step 10: Send email headers and body */
    char date_header[128];
    if (format_email_date(date_header, sizeof(date_header)) != SUCCESS) {
        strncpy(date_header, "", sizeof(date_header));
    }
    
    char email_data[8192];
    size_t offset = 0;
    
    /* Email headers */
    offset += snprintf(email_data + offset, sizeof(email_data) - offset,
                      "From: Deadlock Detector <%s>\r\n", from_email);
    offset += snprintf(email_data + offset, sizeof(email_data) - offset,
                      "To: %s\r\n", to_email);
    offset += snprintf(email_data + offset, sizeof(email_data) - offset,
                      "Subject: %s\r\n", subject);
    if (date_header[0] != '\0') {
        offset += snprintf(email_data + offset, sizeof(email_data) - offset,
                          "Date: %s\r\n", date_header);
    }
    offset += snprintf(email_data + offset, sizeof(email_data) - offset, "\r\n");
    
    /* Email body */
    size_t body_len = strlen(body);
    if (offset + body_len + 10 < sizeof(email_data)) {
        memcpy(email_data + offset, body, body_len);
        offset += body_len;
    } else {
        /* Truncate if too long */
        size_t remaining = sizeof(email_data) - offset - 10;
        memcpy(email_data + offset, body, remaining);
        offset += remaining;
    }
    
    /* End of DATA section */
    offset += snprintf(email_data + offset, sizeof(email_data) - offset, "\r\n.\r\n");
    
    /* Send email data */
    ssize_t sent = send(sock, email_data, offset, 0);
    if (sent < 0) {
        error_log("Failed to send email data: %s", strerror(errno));
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Read response to DATA */
    received = recv(sock, response, sizeof(response) - 1, 0);
    if (received < 0) {
        error_log("Failed to receive DATA response: %s", strerror(errno));
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    response[received] = '\0';
    
    response_code = parse_smtp_response(response);
    if (response_code != 250) {
        error_log("SMTP DATA response failed: %s (code: %d)", response, response_code);
        close(sock);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    /* Step 11: Send QUIT */
    send_smtp_command(sock, "QUIT\r\n", response, sizeof(response));
    
    close(sock);
    return SUCCESS;
}

