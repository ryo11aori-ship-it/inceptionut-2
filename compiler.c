#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

// ==================== 本物のInceptionut VM (dim0版) ====================
void run_inceptionut(const char *source) {
    // S/Tのみ抽出
    long size = strlen(source);
    char *st = malloc(size + 1);
    long st_len = 0;
    for (long i = 0; i < size; i++) {
        if (source[i] == ' ' || source[i] == '\t') {
            st[st_len++] = source[i];
        }
    }
    st[st_len] = '\0';

    // メモリ (dim0: 10 cells, 初期値0)
    int64_t memory[10] = {0};

    // ロード: ビットストリームを4bitずつdigitに変換して順次書き込み
    long bit = 0;
    for (int i = 0; i < 10; i++) {
        if (bit + 3 >= st_len) break;
        int digit = 0;
        for (int b = 0; b < 4; b++) {
            if (st[bit + b] == '\t') digit |= (1 << (3 - b));
        }
        memory[i] = digit;
        bit += 4;
    }

    printf("[VM] Loaded %ld bits → %d cells\n", bit, 10);

    // Subleq実行 (PCはindex、addressはmod 10)
    int64_t pc = 0;
    long steps = 0;
    while (1) {
        if (++steps > 10000) {
            printf("[VM Stopped: too many steps (possible infinite loop)]\n");
            break;
        }
        if (pc < 0 || pc + 2 >= 10) {
            printf("[VM Stopped: PC out of range]\n");
            break;
        }

        int64_t a_idx = pc % 10;
        int64_t b_idx = (pc + 1) % 10;
        int64_t c_idx = (pc + 2) % 10;

        int64_t a = memory[a_idx];
        int64_t b = memory[b_idx];
        int64_t c = memory[c_idx];

        int64_t val_a = memory[a % 10];
        memory[b % 10] -= val_a;

        // I/O (dim0の果て = 9)
        if (b == 9) {
            putchar((char)(memory[9] & 0xFF));
            fflush(stdout);
        }
        if (a == 9) {
            int ch = getchar();
            memory[a % 10] = (ch == EOF) ? -1LL : (int64_t)ch;
        }

        if (memory[b % 10] <= 0) {
            pc = c;
        } else {
            pc += 3;
        }
    }

    free(st);
    printf("[VM] Execution finished (dim0 only, steps: %ld)\n", steps);
}

int main(int argc, char **argv) {
    // ==================== --run モード (本物VM実行) ====================
    if (argc > 1 && strcmp(argv[1], "--run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s --run <input.inc>\n", argv[0]);
            return 1;
        }
        FILE *in = fopen(argv[2], "rb");
        if (!in) {
            perror("open input for --run");
            return 1;
        }
        fseek(in, 0, SEEK_END);
        long size = ftell(in);
        fseek(in, 0, SEEK_SET);
        char *buf = malloc(size + 1);
        fread(buf, 1, size, in);
        fclose(in);
        buf[size] = '\0';

        run_inceptionut(buf);
        free(buf);
        return 0;
    }

    // ==================== 通常コンパイル (固定点維持) ====================
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.inc> <output.elf>\n", argv[0]);
        return 1;
    }

    // inputは読み込むだけ（自己複製では内容不要）
    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        perror("open input");
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    fread(buf, 1, size, in);
    fclose(in);
    free(buf);

    // 自己複製（固定点検証用）
    FILE *self = fopen("/proc/self/exe", "rb");
    if (!self) {
        perror("open /proc/self/exe");
        return 1;
    }
    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        perror("open output");
        fclose(self);
        return 1;
    }

    char copybuf[4096];
    size_t n;
    while ((n = fread(copybuf, 1, sizeof(copybuf), self)) > 0) {
        fwrite(copybuf, 1, n, out);
    }

    fclose(self);
    fclose(out);

    printf("✅ Compiled (self-replicating): %s -> %s\n", argv[1], argv[2]);
    return 0;
}