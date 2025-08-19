#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sha256.h"   // your implementation (sha256.c)

// Use system OpenSSL CLI to compute SHA-256
void run_openssl_sha256(const char *input, char *openssl_output) {
    char command[2048];
    snprintf(command, sizeof(command), "echo -n '%s' | openssl dgst -sha256 | awk '{print $2}'", input);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        strcpy(openssl_output, "ERROR");
        return;
    }
    if (fgets(openssl_output, 65, fp) == NULL) {
        strcpy(openssl_output, "ERROR");
    }
    pclose(fp);

    // Trim newline if present
    openssl_output[strcspn(openssl_output, "\n")] = '\0';
}

#define MAX_INPUT 256

int main(void) {
    const int screenWidth = 900;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "SHA-256 Checker (Raylib GUI)");

    char inputText[MAX_INPUT] = {0};
    int letterCount = 0;

    bool checkPressed = false;
    char myHash[65] = {0};
    char refHash[65] = {0};
    char resultMsg[128] = {0};

    struct sha256_ctx ctx;
    unsigned char digest[32];

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Handle text input
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT - 1)) {
                inputText[letterCount] = (char)key;
                inputText[letterCount + 1] = '\0';
                letterCount++;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (letterCount > 0) {
                letterCount--;
                inputText[letterCount] = '\0';
            }
        }

        // Check button or Enter key
        if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), (Rectangle){ 700, 50, 120, 40 }))) {
            
            // Compute using your SHA-256
            sha256_init(&ctx);
            sha256_update(&ctx, (const u8 *)inputText, strlen(inputText));
            sha256_final(&ctx, digest);
            sha256_to_hex(digest, myHash);

            // Compute using system OpenSSL
            run_openssl_sha256(inputText, refHash);

            if (strcmp(myHash, refHash) == 0) {
                strcpy(resultMsg, "Hashes match!");
            } else {
                strcpy(resultMsg, "Mismatch!");
            }

            checkPressed = true;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Enter text:", 50, 50, 20, DARKGRAY);
        DrawRectangleLines(200, 50, 450, 40, GRAY);
        DrawText(inputText, 210, 60, 20, BLACK);

        // Check button
        DrawRectangle(700, 50, 120, 40, LIGHTGRAY);
        DrawText("Check", 730, 60, 20, BLACK);

        if (checkPressed) {
            DrawText("Your SHA-256:", 50, 150, 20, DARKGRAY);
            DrawText(myHash, 50, 180, 20, BLACK);

            DrawText("OpenSSL SHA-256:", 50, 240, 20, DARKGRAY);
            DrawText(refHash, 50, 270, 20, BLACK);

            DrawText(resultMsg, 50, 330, 30, 
                (strcmp(myHash, refHash) == 0) ? DARKGREEN : RED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
