#include <stdio.h>
#include <stdbool.h>
// #include <linux/types.h> this is the user version: #include <stdbool.h>
#include <string.h>
// #include <linux/string.h> this is the user version: #include <string.h>

typedef char unsigned u8;
typedef short unsigned u16;
typedef int unsigned u32;
typedef long long unsigned u64;
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;
// typedef int bool; for kernel in bool
//#define true 1
// #define false 0 for kernel in bool
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))



enum grp {ax, bx, cx, dx, sp, bp, si, di, ip, ss, ds, es, fl}; // general purpose registers
enum flag_bits {o=4,d=5,i=6,t=7,s=8,z=9,a=11,p=13,c=15};

// transfered to maps
// enum flag_bits_masker {o_masker=0b100000000000,d_masker=0b10000000000,i_masker=0b1000000000,t_masker=0b100000000,s_masker=0b10000000,
//     z_masker=0b1000000,a_masker=0b10000,p_masker=0b100,c_masker=0b1};

u16 general_purpose_registers[16]; // 12 is the flag

enum MnemonicId {
    MN_UNDEF = 0,
    MN_MOV,
    MN_ADD,
    MN_SUB,
    MN_CMP,
    MN_JNE,
    MN_COUNT
};
static const char *mnemonic_names[MN_COUNT] = {
     "UNDEF", "mov", "add", "sub", "cmp", "jne"
};
static u8 mnemonic_index[256];

void init_table(void){
    int i;
    for (i = 0; i < 256; ++i){
        mnemonic_index[i] = MN_UNDEF;
    }
    mnemonic_index[0b100010] = 1;
}

//static const char *mnemonic_names[MN_COUNT] = {
//     "UNDEF", "mov", "add", "sub", "cmp", "jne"
//};
//static u8 reg_index[32];
//void init_register(void){
//
//}


typedef struct{
    u8 key;
    const char *value;
} umap;

typedef struct{
    u8 key;
    const char *value;
} umap_r;

umap nem[] = {
    {0b100010, "mov"}, // Register/memory to/from register
    {0b1100011, "mov"},// immediate to register/memory
    {0b1011, "mov"}, // immediate to register
    {0b1010000, "mov"}, // memory to accumulator
    {0b1010001, "mov"}, // accumulator to memory
    {0b10001110, "mov"}, // Register/memory to segment register
    {0b10001100, "mov"},// Segmenet register to register/memory
    // ----------------------------------------------------------------//
    {0b0, "add"},
    {0b100000, "add"},
    {0b0000010, "add"},
    // ----------------------------------------------------------------//
    {0b001010, "sub"},
    // {0b100000, "sub"},
    {0b0010110, "sub"},
    // ----------------------------------------------------------------//
    {0b001110, "cmp"},
    {0b0011110, "cmp"},
    // ----------------------------------------------------------------//
    {0b01110101, "jne"}
};

//umap reg_w[] = {
//    {0b000, "ax"},
//    {0b001, "cx"},
//    {0b010, "dx"},
//    {0b011, "bx"},
//    // _________________ space for readability
//    {0b100, "sp"},
//    {0b101, "bp"},
//    {0b110, "si"},
//    {0b111, "di"},
//};

static const char *reg_w[8] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

static const char *reg_nw[8] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

//umap reg_nw[] = {
//    {0b000, "al"},
//    {0b001, "cl"},
//    {0b010, "dl"},
//    {0b011, "bl"},
//    // _________________ space for readability
//    {0b100, "ah"},
//    {0b101, "ch"},
//    {0b110, "dh"},
//    {0b111, "bh"},
//};

//umap rm[] = {
//    {0b000, "bx + si"},
//    {0b001, "bx + di"},
//    {0b010, "bp + si"},
//    {0b011, "bp + di"},
//    // _________________ space for readability
//    {0b100, "si"},
//    {0b101, "di"},
//    {0b110, "bp"}, // if mod == 00 it is direct address
//    {0b111, "bx"},
//};

static const char *rm[8] = {
    "bx + si", /* 0 */
    "bx + di", /* 1 */
    "bp + si", /* 2 */
    "bp + di", /* 3 */
    "si",      /* 4 */
    "di",      /* 5 */
    "bp",      /* 6 -> special when mod == 0 */
    "bx"       /* 7 */
};

void copy_string(char *dest, const char *src, size_t max_len) {
    int i = 0;
    while (src[i] != '\0' && i < (int)max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0'; // null terminate
}

void left_and_right_encoding(u8 champiArray[], u8 counter[], char* out){
    u16 index = (u16)counter[0];
    u8 input = 0 + champiArray[index];

    for (int i = 0; i < 8; ++i) {
        // MN_UNDEF
        if(mnemonic_index[input] != MN_UNDEF) { // if shit is not equal to zero it exists. otherwise tapyas pa
            break;
        }

        input = input >> 1;
    }

    char cur_nem[] = mnemonic_names[mnemonic_index[input]];

    switch(input){
        case 0b100010:  {
            // left encoding
            bool destination = (0b00000010 & champiArray[index]) << 1;
            bool wide = (0b00000001 & champiArray[index]);

            // right encoding
            u8 input2 = champiArray[index + 1];
            u8 input_mod = (0b11000000 & input2) >> 6; // not yet implemented
            u8 input_reg = (0b00111000 & input2) >> 3;
            u8 input_rm = (0b00000111 & input2);

            char s_reg[256];
            char s_rm[256];

            switch (input_mod) {
                case 0b11: {
                    if( wide ){
                        copy_string(s_reg, reg_w[input_mod] ,sizeof(s_reg));
                        copy_string(s_rm, reg_w[input_rm] ,sizeof(s_rm));
                    }else {
                        copy_string(s_reg, reg_nw[input_mod] ,sizeof(s_reg));
                        copy_string(s_rm, reg_nw[input_rm] ,sizeof(s_rm));
                    }

                    if( destination ){
                        strncat(out, s_reg , sizeof(out) - strlen(out) - 1);
                        strncat(out, s_reg , sizeof(out) - strlen(out) - 1);
                        strncat(out, s_rm , sizeof(out) - strlen(out) - 1);
                        return;
                    }
                    else{
                        strncat(out, s_rm , sizeof(out) - strlen(out) - 1);
                        strncat(out, s_reg , sizeof(out) - strlen(out) - 1);
                        return;
                    }

                } break;
            } break;



        } break;

    };

}


// int main(int argc, char **argv)
int main()
{
    init_table();
    // 10001001 11011001
    u8 champiHex[] = {0b10001001, 0b11011001, 0b10001001, 0b11011001, 0b10001001, 0b11011001};
    char result[256];
    u8 kopal[] = {0, 0};
    while(kopal[0] < ArrayCount(champiHex)){
        char labi[] = "WWWWWW "; // placeholder
        left_and_right_encoding(champiHex, kopal, result);

        //copy_string(result, mnemonic_names[MN_MOV], sizeof(result));
        kopal[0]+=2; // minimum i think
        printf("sadasdasd %s\n", result);
        // printf("asdasdads %s %d\n", result, mnemonic_index[MN_MOV]);
    }

    // printf("Hello: %s number:%d\n", mnemonic_names[MN_MOV], MN_MOV);
    //printf("jamich: \n", );
    printf("Hello world! %s\n", result);
    return 0;
}






















