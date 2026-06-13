#include <openssl/base64.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    char text[] = "hello, world!";
    size_t out_len;
    assert(1 == EVP_EncodedLength(&out_len, sizeof(text)));
    uint8_t* buf = calloc(out_len, sizeof(char));
    assert(NULL != buf);
    size_t ret = EVP_EncodeBlock(buf, (const uint8_t *)text, sizeof(text));
    assert(ret > 0);
    printf("%s\n", buf);
    return 0;
}
