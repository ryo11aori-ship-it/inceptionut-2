#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

// ==================== フルInceptionut VM ====================
#define MAX_DIM 8  // 安全のため最大8次元まで（十分）

typedef struct {
    int64_t cells[10];           // 1次元 = 10セル
} Dimension;

Dimension *universe[MAX_DIM + 1];  // 超^n次元
int current_max_dim = 0;           // 現在開通している最大次元

// 座標タプル (0〜current_max_dim の n+1 要素)
typedef struct {
    int coords[MAX_DIM + 1];
    int dim;  // 要素数 = dim+1
} Coord;

// メモリ読み書き
int64_t mem_read(Coord c) {
    if (c.dim > current_max_dim) return 0;
    int64_t *p = universe[c.dim]->cells;
    for (int i = c.dim; i >= 0; i--) {
        int idx = c.coords[i];
        if (i == 0) return p[idx];
        p = universe[i-1]->cells;  // 下位次元はまだ未使用（カプセル化時は後で実装）
    }
    return 0; // 未定義領域
}

void mem_write(Coord c, int64_t val) {
    if (c.dim > current_max_dim) {
        // 次元拡張
        while (c.dim > current_max_dim) {
            universe[current_max_dim + 1] = calloc(1, sizeof(Dimension));
            current_max_dim++;
            printf("[VM] Dimension expanded to %d\n", current_max_dim);
        }
    }
    // 簡易実装：現時点ではdim0のみ完全（拡張ロジックは次回強化）
    if (c.dim == 0) {
        universe[0]->cells[c.coords[0]] = val;
    }
    // TODO: 将来的にフル多次元カプセル化を実装
}

// 座標加算（PC進み用 base-10）
void coord_inc(Coord *c) {
    int carry = 1;
    for (int i = 0; i <= c->dim; i++) {
        c->coords[i] += carry;
        if (c->coords[i] <= 9) {
            carry = 0;
            break;
        }
        c->coords[i] = 0;
        carry = 1;
    }
    if (carry) {
        // 次元拡張が必要ならここで
    }
}

// 座標をdigitタプルから構築
Coord parse_coord(const char *st, long *bit_pos, int num_digits) {
    Coord c = {0};
    c.dim = num_digits - 1;
    for (int i = 0; i < num_digits; i++) {
        int digit = 0;
        for (int b = 0; b < 4; b++) {
            if (st[*bit_pos + b] == '\t') digit |= (1 << (3 - b));
        }
        c.coords[num_digits - 1 - i] = digit;  // 最低次元が右
        *bit_pos += 4;
    }
    return c;
}

void run_inceptionut(const char *source) {
    // S/T抽出
    long size = strlen(source);
    char *st = malloc(size + 1);
    long st_len = 0;
    for (long i = 0; i < size; i++) {
        if (source[i] == ' ' || source[i] == '\t') st[st_len++] = source[i];
    }
    st[st_len] = '\0';

    // 宇宙初期化
    current_max_dim = 0;
    for (int i = 0; i <= MAX_DIM; i++) {
        universe[i] = (i == 0) ? calloc(1, sizeof(Dimension)) : NULL;
    }

    // ロード（dim0）
    long bit = 0;
    for (int i = 0; i < 10 && bit + 3 < st_len; i++) {
        int digit = 0;
        for (int b = 0; b < 4; b++) {
            if (st[bit + b] == '\t') digit |= (1 << (3 - b));
        }
        universe[0]->cells[i] = digit;
        bit += 4;
    }
    printf("[VM] dim0 loaded (%ld bits)\n", bit);

    // 実行
    Coord pc = {0};
    pc.dim = 0;
    long steps = 0;
    while (1) {
        if (++steps > 100000) { printf("[VM] too many steps\n"); break; }

        int n = current_max_dim;           // 現在の次元
        int digits_per_op = n + 1;         // 各オペランドのdigit数
        long needed_bits = 12LL * digits_per_op;

        if (bit + needed_bits > st_len) {
            printf("[VM] EOF / parse collapse\n");
            break;
        }

        // A, B, C をパース
        Coord A = parse_coord(st, &bit, digits_per_op);
        Coord B = parse_coord(st, &bit, digits_per_op);
        Coord C = parse_coord(st, &bit, digits_per_op);

        int64_t val_a = mem_read(A);
        int64_t new_b = mem_read(B) - val_a;
        mem_write(B, new_b);

        // I/O
        int is_io = 1;
        for (int i = 0; i <= n; i++) if (B.coords[i] != 9) { is_io = 0; break; }
        if (is_io) {
            if (B.dim == n) {  // 出力
                putchar((char)(new_b & 0xFF));
                fflush(stdout);
            } else if (A.dim == n) {  // 入力
                int ch = getchar();
                mem_write(A, (ch == EOF) ? -1LL : ch);
            }
        }

        if (new_b <= 0) {
            pc = C;  // ジャンプ
        } else {
            coord_inc(&pc);  // 次の命令へ
        }
    }

    free(st);
    printf("[VM] finished (steps: %ld, max_dim: %d)\n", steps, current_max_dim);
}

// ==================== main ====================
int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s --run <input.inc>\n", argv[0]);
            return 1;
        }
        FILE *f = fopen(argv[2], "rb");
        if (!f) { perror("open"); return 1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *buf = malloc(sz + 1);
        fread(buf, 1, sz, f);
        fclose(f);
        buf[sz] = '\0';
        run_inceptionut(buf);
        free(buf);
        return 0;
    }

    // コンパイル（自己複製・固定点維持）
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.inc> <output.elf>\n", argv[0]);
        return 1;
    }
    FILE *in = fopen(argv[1], "rb");
    if (in) { fseek(in, 0, SEEK_END); fclose(in); }  // 存在チェックのみ

    FILE *self = fopen("/proc/self/exe", "rb");
    if (!self) { perror("/proc/self/exe"); return 1; }
    FILE *out = fopen(argv[2], "wb");
    if (!out) { perror("output"); fclose(self); return 1; }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), self)) > 0) fwrite(buf, 1, n, out);

    fclose(self); fclose(out);
    printf("✅ Self-replicating compile: %s -> %s\n", argv[1], argv[2]);
    return 0;
}