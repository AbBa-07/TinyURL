
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mongoose.h"
#include "tinyurl.h"

#define DEFAULT_PORT "8000"

static void sendHomePage(struct mg_connection *c)
{
    const char *html =
        "<!DOCTYPE html><html><head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>TinyURL</title>"
        "<style>"
        "body{font-family:Arial;background:#f2f4f8;text-align:center;padding:50px 15px;}"
        ".box{background:white;padding:35px;border-radius:14px;max-width:500px;"
        "margin:auto;box-shadow:0 4px 15px #ccc;}"
        "input{width:90%;padding:13px;margin:15px 0;border:1px solid #ccc;"
        "border-radius:6px;box-sizing:border-box;font-size:16px;}"
        "button{padding:13px 22px;background:#2563eb;color:white;border:0;"
        "border-radius:6px;cursor:pointer;font-size:16px;}"
        "button:hover{background:#1d4ed8;}"
        "</style></head><body>"
        "<div class='box'><h1>TinyURL</h1>"
        "<p>Make your long URLs short and simple.</p>"
        "<form method='POST' action='/shorten'>"
        "<input type='url' name='url' placeholder='Paste your long URL here' required>"
        "<br><button type='submit'>Shorten URL</button>"
        "</form></div></body></html>";

    mg_http_reply(c, 200,
        "Content-Type: text/html; charset=utf-8\r\n",
        "%s", html);
}

static int getPublicBaseURL(struct mg_http_message *hm,
                            char baseURL[], size_t size)
{
    const struct mg_str *host = mg_http_get_header(hm, "Host");

    if (host == NULL || host->len == 0 || host->len > 250)
        return 0;

    char hostText[251];

    memcpy(hostText, host->buf, host->len);
    hostText[host->len] = '\0';

    /* Accept only ordinary hostname and port characters. */
    for (size_t i = 0; i < host->len; i++)
    {
        char ch = hostText[i];

        if (!((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '.' || ch == '-' || ch == ':' ||
              ch == '[' || ch == ']'))
            return 0;
    }

    const struct mg_str *forwarded =
        mg_http_get_header(hm, "X-Forwarded-Proto");

    const char *scheme = "http";

    if (forwarded != NULL &&
        forwarded->len == 5 &&
        memcmp(forwarded->buf, "https", 5) == 0)
    {
        scheme = "https";
    }

    int written = snprintf(baseURL, size, "%s://%s", scheme, hostText);

    return written > 0 && (size_t)written < size;
}

static void handleRequest(struct mg_connection *c, int ev, void *ev_data)
{
    if (ev != MG_EV_HTTP_MSG)
        return;

    struct mg_http_message *hm = (struct mg_http_message *)ev_data;

    /* Homepage */
    if (mg_match(hm->uri, mg_str("/"), NULL) &&
        mg_match(hm->method, mg_str("GET"), NULL))
    {
        sendHomePage(c);
        return;
    }

    /* Create short URL */
    if (mg_match(hm->uri, mg_str("/shorten"), NULL) &&
        mg_match(hm->method, mg_str("POST"), NULL))
    {
        char originalURL[500];
        char shortCode[20];
        char baseURL[270];
        char shortURL[300];

        int length = mg_http_get_var(
            &hm->body, "url", originalURL, sizeof(originalURL));

        if (length <= 0 ||
            !createURL(originalURL, shortCode))
        {
            mg_http_reply(c, 400,
                "Content-Type: text/html; charset=utf-8\r\n",
                "<h2>Invalid URL</h2>"
                "<p>Please enter a valid HTTP or HTTPS URL.</p>"
                "<a href='/'>Go back</a>");
            return;
        }

        if (!getPublicBaseURL(hm, baseURL, sizeof(baseURL)))
        {
            mg_http_reply(c, 400,
                "Content-Type: text/plain\r\n",
                "Could not determine website address.\n");
            return;
        }

        snprintf(shortURL, sizeof(shortURL),
                 "%s/%s", baseURL, shortCode);

        char html[2500];

        snprintf(html, sizeof(html),
            "<!DOCTYPE html><html><head>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<title>Your TinyURL</title>"
            "<style>"
            "body{font-family:Arial;background:#f2f4f8;text-align:center;padding:50px 15px;}"
            ".box{background:white;padding:30px;border-radius:14px;max-width:500px;"
            "margin:auto;box-shadow:0 4px 15px #ccc;}"
            ".url{overflow-wrap:anywhere;color:#2563eb;font-weight:bold;}"
            "button{padding:12px 20px;background:#2563eb;color:white;border:0;"
            "border-radius:6px;cursor:pointer;margin:8px;}"
            "</style></head><body><div class='box'>"
            "<h1>Your short URL is ready!</h1>"
            "<p class='url' id='shortUrl'>%s</p>"
            "<button onclick='copyURL()'>Copy Short URL</button>"
            "<p id='copyMessage'></p>"
            "<p>Short code: %s</p>"
            "<a href='/'>Shorten another URL</a>"
            "</div>"
            "<script>"
            "function copyURL(){"
            "var url=document.getElementById('shortUrl').textContent;"
            "navigator.clipboard.writeText(url).then(function(){"
            "document.getElementById('copyMessage').textContent='Copied!';"
            "}).catch(function(){"
            "document.getElementById('copyMessage').textContent='Copy failed. Please copy the link manually.';"
            "});}"
            "</script></body></html>",
            shortURL, shortCode);

        mg_http_reply(c, 200,
            "Content-Type: text/html; charset=utf-8\r\n",
            "%s", html);

        return;
    }

    /* Redirect short URL */
    if (hm->uri.len > 1 && hm->uri.len < 20 &&
        hm->uri.buf[0] == '/' &&
        mg_match(hm->method, mg_str("GET"), NULL))
    {
        char shortCode[20];

        size_t codeLength = hm->uri.len - 1;

        memcpy(shortCode, hm->uri.buf + 1, codeLength);
        shortCode[codeLength] = '\0';

        int validCode = 1;

        for (size_t i = 0; i < codeLength; i++)
        {
            if (shortCode[i] < '0' || shortCode[i] > '9')
            {
                validCode = 0;
                break;
            }
        }

        if (validCode)
        {
            struct URLRecord *record = searchURL(shortCode);

            if (record != NULL)
            {
                record->clickCount++;
                saveData();

                char headers[600];

                snprintf(headers, sizeof(headers),
                    "Location: %s\r\nCache-Control: no-store\r\n",
                    record->originalURL);

                mg_http_reply(c, 302, headers, "");
                return;
            }
        }
    }

    mg_http_reply(c, 404,
        "Content-Type: text/html; charset=utf-8\r\n",
        "<h1>404 - Page Not Found</h1><a href='/'>Home</a>");
}

static void eventHandler(struct mg_connection *c, int ev,
                         void *ev_data)
{
    handleRequest(c, ev, ev_data);
}

int main(void)
{
    struct mg_mgr mgr;
    char address[100];

    const char *port = getenv("PORT");

    if (port == NULL || port[0] == '\0')
        port = DEFAULT_PORT;

    snprintf(address, sizeof(address),
             "http://0.0.0.0:%s", port);

    initializeTable();
    loadData();

    mg_mgr_init(&mgr);

    struct mg_connection *connection =
        mg_http_listen(&mgr, address, eventHandler, NULL);

    if (connection == NULL)
    {
        printf("Could not start server at %s!\n", address);
        mg_mgr_free(&mgr);
        freeTable();
        return 1;
    }

    printf("\nTinyURL website is running!\n");
    printf("Server address: %s\n", address);

    for (;;)
        mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    freeTable();

    return 0;
}
