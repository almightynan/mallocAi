/***************************************************************************
 * 
 *  mallocAi.h - Header for mallocAi library
 *  author: almightynan [https://almightynan.cc]
 *
 * AI-guided memory allocator that communicates with a local AI server.
 * Sends prompts to localhost:3000/gemini over HTTP, expects JSON reply
 * of the form: {"text": "<number>"}. Resulting number is malloc'd.
 *
 * Header defines two interfaces:
 *   - mallocAi(prompt): calls server and allocates, silently.
 *   - mallocAi_verbose(prompt, verbose): same but logs size if verbose=1.
 *
 * WARNING: If the server replies with "undefined", "infinite", or invalid
 * values, the allocator will intentionally crash with a segmentation fault.
 *
 * Intended for creative misuse, testing absurd ideas, or deliberate failure.
 * Not designed for safety, performance, or serious use cases.
 *
 * No guarantees. No warranty. No reason. Use at your own existential risk.
 ***************************************************************************/

#ifndef MALLOCAI_H
#define MALLOCAI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ## mallocAi() - Allocate memory based on AI response.
 * @param prompt prompt sent to the AI server.
 *
 * @note By default, verbose output is disabled.
 * @note To enable verbose output showing allocation size, use mallocAi_verbose().
 *
 * @returns pointer to allocated memory, or NULL on failure.
 */
void *mallocAi(const char *prompt);

/**
 * ## mallocAi_verbose() - Allocate memory and optionally show allocation size.
 * @param prompt prompt sent to the AI server.
 * @param verbose set to 1 to enable verbose output, 0 to disable.
 *
 * Use this function to see how many bytes mallocAi has chosen to allocate.
 *
 * @returns: pointer to allocated memory, or NULL on failure.
 */

void *mallocAi_verbose(const char *prompt, int verbose);

#ifdef __cplusplus
}
#endif

#endif
