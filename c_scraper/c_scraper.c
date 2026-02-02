#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define USER_AGENT "CyberResearchBot/1.0 (Academic project; contact: jaxsa.luthor@gmail.com)"

/* Structure to hold response data */
typedef struct {
    char *data;
    size_t size;
} ResponseData;

/* Callback to capture response body */
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    ResponseData *response = (ResponseData *)userdata;
    
    char *temp = realloc(response->data, response->size + total_size + 1);
    if (!temp) {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }
    
    response->data = temp;
    memcpy(&(response->data[response->size]), ptr, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';
    
    return total_size;
}

/* Check if a tag is in our list of tags to extract */
int is_target_tag(const char *tag, const char *tags[], int tag_count) {
    for (int i = 0; i < tag_count; i++) {
        if (strcasecmp(tag, tags[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Extract tag name from opening tag */
char* get_tag_name(const char *tag_start) {
    const char *ptr = tag_start + 1; // Skip '<'
    const char *end = ptr;
    
    // Find end of tag name (space or >)
    while (*end && *end != ' ' && *end != '>' && *end != '/') {
        end++;
    }
    
    size_t len = end - ptr;
    char *tag_name = malloc(len + 1);
    if (tag_name) {
        strncpy(tag_name, ptr, len);
        tag_name[len] = '\0';
    }
    return tag_name;
}

/* Extract tags in order they appear */
void extract_tags_in_order(const char *html, const char *tags[], int tag_count) {
    const char *ptr = html;
    
    printf("\n========== Extracted Content ==========\n\n");
    
    while (*ptr) {
        // Find next opening tag
        const char *tag_start = strchr(ptr, '<');
        if (!tag_start) break;
        
        // Skip closing tags and comments
        if (tag_start[1] == '/' || tag_start[1] == '!') {
            ptr = tag_start + 1;
            continue;
        }
        
        // Get tag name
        char *tag_name = get_tag_name(tag_start);
        if (!tag_name) {
            ptr = tag_start + 1;
            continue;
        }
        
        // Check if this is a tag we want
        if (is_target_tag(tag_name, tags, tag_count)) {
            // Find end of opening tag
            const char *tag_close = strchr(tag_start, '>');
            if (!tag_close) {
                free(tag_name);
                break;
            }
            
            // Build closing tag
            char close_tag[64];
            snprintf(close_tag, sizeof(close_tag), "</%s>", tag_name);
            
            // Find closing tag
            const char *tag_end = strcasestr(tag_close + 1, close_tag);
            if (tag_end) {
                // Extract content
                size_t content_len = tag_end - (tag_close + 1);
                char *content = malloc(content_len + 1);
                if (content) {
                    strncpy(content, tag_close + 1, content_len);
                    content[content_len] = '\0';
                    
                    // Print the full tag with content
                    printf("<%s>%s</%s>\n\n", tag_name, content, tag_name);
                    free(content);
                }
                
                ptr = tag_end + strlen(close_tag);
            } else {
                ptr = tag_start + 1;
            }
        } else {
            ptr = tag_start + 1;
        }
        
        free(tag_name);
    }
}

int reqrep(const char *target) {
    CURL *curl;
    CURLcode result;
    ResponseData response = {NULL, 0};

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "❌ curl init failed\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, target);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    result = curl_easy_perform(curl);

    if (result == CURLE_OK) {
        printf("✓ Request successful\n");
        
        // Specify which tags to extract (in order they appear in HTML)
        const char *tags_to_extract[] = {"h1", "h2", "h3", "p"};
        int tag_count = sizeof(tags_to_extract) / sizeof(tags_to_extract[0]);
        
        extract_tags_in_order(response.data, tags_to_extract, tag_count);
        
    } else {
        fprintf(stderr, "❌ curl error: %s\n", curl_easy_strerror(result));
    }

    curl_easy_cleanup(curl);
    free(response.data);
    return 0;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    reqrep("https://en.wikipedia.org/wiki/Computer");
    curl_global_cleanup();
    return 0;
}
