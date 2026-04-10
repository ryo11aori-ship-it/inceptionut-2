#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.inc> <output.elf>\n", argv[0]);
        return 1;
    }

    // .inc読み込み → S/Tのみ抽出（他文字は無視）
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

    char *source = malloc(size + 1);
    long src_len = 0;
    for (long i = 0; i < size; i++) {
        if (buf[i] == ' ' || buf[i] == '\t') {
            source[src_len++] = buf[i];
        }
    }
    source[src_len] = '\0';
    free(buf);

    // === ELF生成（最小x86_64 ELF + sourceをdata sectionに埋め込み）===
    // 将来ここにフルVM（Subleq実行部）を入れる予定。現在はexit(0)スタブ
    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        perror("open output");
        return 1;
    }

    // ELFヘッダ + プログラムヘッダ（PT_LOAD 1つでcode+dataをカバー）
    unsigned char elf[0x80] = {0}; // 初期化

    // ELF header (64byte)
    memcpy(elf, "\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
    elf[16] = 2; elf[17] = 0;   // ET_EXEC
    elf[18] = 0x3e; elf[19] = 0; // x86_64
    elf[24] = 0x78; elf[25] = 0; elf[26] = 0x40; // e_entry = 0x400078
    elf[32] = 0x40; // e_phoff = 0x40
    elf[40] = 0x38; elf[42] = 1; // e_phentsize=56, e_phnum=1

    // Program header (PT_LOAD, offset 0, vaddr 0x400000)
    uint64_t code_vaddr = 0x400000;
    uint64_t code_offset = 0x78;           // コード開始オフセット
    uint64_t code_size = 10;               // exitコード長
    uint64_t data_offset = code_offset + code_size;
    uint64_t total_size = data_offset + src_len;

    // PH: type=PT_LOAD, flags=R+X, offset=0, vaddr=0x400000
    uint8_t *ph = elf + 0x40;
    ph[0] = 1; ph[4] = 5; // PT_LOAD, PF_R|PF_X
    // filesz / memsz を source込みで更新
    *(uint64_t*)(ph + 0x10) = total_size;  // p_filesz
    *(uint64_t*)(ph + 0x28) = total_size;  // p_memsz
    *(uint64_t*)(ph + 0x08) = 0;           // p_offset = 0

    fwrite(elf, 1, 0x80, out);

    // コード部（exit(0)）
    unsigned char exit_code[10] = {
        0xb0, 0x3c,             // mov al, 60
        0x48, 0x31, 0xff,       // xor rdi, rdi
        0x0f, 0x05              // syscall
    };
    fwrite(exit_code, 1, 10, out);

    // data section に source (S/T) を埋め込み
    fwrite(source, 1, src_len, out);

    fclose(out);
    free(source);

    printf("Compiled: %s -> %s (source bytes: %ld)\n", argv[1], argv[2], src_len);
    return 0;
}