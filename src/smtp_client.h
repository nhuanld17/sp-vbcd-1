#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

/* =============================================================================
 * SMTP_CLIENT.H - SMTP Client Interface (Pure C)
 * =============================================================================
 * Direct SMTP protocol implementation using raw sockets.
 * No external libraries - only standard C and POSIX socket API.
 * =============================================================================
 */

#include "config.h"

/* =============================================================================
 * FUNCTION PROTOTYPES
 * =============================================================================
 */

/*
 * send_email_via_smtp - Send email via direct SMTP connection
 * @smtp_server: SMTP server hostname (e.g., "localhost" for local SMTP)
 * @smtp_port: SMTP server port (e.g., 25 for plain SMTP)
 * @from_email: Sender email address
 * @to_email: Recipient email address (comma-separated for multiple)
 * @subject: Email subject
 * @body: Email body content
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Establishes TCP connection to SMTP server and sends email
 *              using raw SMTP protocol commands. Handles basic SMTP flow:
 *              HELO, MAIL FROM, RCPT TO, DATA, QUIT.
 *              Uses plain SMTP (no STARTTLS/TLS encryption).
 *              Recommended: Use localhost:25 with local Postfix server.
 *              Time complexity: O(1) network operations
 * Error handling: Returns error codes for connection failures, SMTP errors.
 *                 Shows helpful message if localhost:25 is not available.
 */
int send_email_via_smtp(const char *smtp_server, int smtp_port,
                        const char *from_email, const char *to_email,
                        const char *subject, const char *body);

/*
 * parse_smtp_response - Parse SMTP server response code
 * @response: SMTP server response string
 * @return: Response code (e.g., 220, 250) or -1 on parse error
 * Description: Extracts numeric response code from SMTP response line.
 *              SMTP responses start with 3-digit code (e.g., "220 OK").
 * Error handling: Returns -1 if response format is invalid
 */
int parse_smtp_response(const char *response);

/*
 * send_smtp_command - Send command to SMTP server and read response
 * @sock: Socket file descriptor
 * @command: SMTP command to send (e.g., "HELO localhost\r\n")
 * @response: Buffer to store server response
 * @response_size: Size of response buffer
 * @return: Response code (e.g., 250) on success, -1 on error
 * Description: Sends SMTP command and waits for server response.
 *              Handles socket I/O and response parsing.
 * Error handling: Returns -1 on send/recv failure or invalid response
 */
int send_smtp_command(int sock, const char *command,
                      char *response, size_t response_size);

#endif /* SMTP_CLIENT_H */

