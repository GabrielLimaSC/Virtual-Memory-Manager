#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PAGE_BYTE_SIZE 256
#define MAX_ADDRESS 65536
#define DECIMAL_TO_BINARY 1
#define BINARY_TO_DECIMAL 2
#define SEPARATE_FALSE 3
#define SEPARATE_TRUE 4

typedef struct page {
    int virtual_address;
    int binary_address[17];
    int number1;
    int number2;
    int binary_to_decimal;
    int TLB;
    int page_table;
    int offset;
    int physical_address;
    int value;
    struct page *next;

} page;

void read_file(char *filename, page **list) {
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {exit(1);}

    int tlb = 0;
    
    char line[20]; 
    while (fgets(line, sizeof(line), file)) {

        page *new_page = (page *)malloc(sizeof(page));
        if (new_page == NULL) {exit(1);}
        
        new_page->virtual_address = atoi(line);
        new_page->TLB = tlb;
        tlb++;
        if (new_page->TLB == 16) new_page->TLB = 0, tlb = 1;
        new_page->next = *list;
        *list = new_page;
    }

    fclose(file);
}

void convert(page *list, int arg, int arg2) {

    if (arg == 1) { // converte de decimal para binário
        while (list != NULL) {
            list->binary_address[16] = '\0';
            int n = list->virtual_address;

            for (int i = 15; i >= 0; i--) {
                list->binary_address[i] = (n % 2) + '0';
                n /= 2;
            }

            list = list->next;
        } 
    } else if (arg == 2) { // converte de binário para decimal

        if (arg2 == 4){ // separa binario em dois numeros e converte para decimal
            while (list != NULL) {
                int number1 = 0;
                int number2 = 0;
                int j = 7;

                for (int i = 0; i < 16; i++) {

                    if (i < 8) {
                        number1 += (list->binary_address[i] - '0') * pow(2, j);
                        if (i == 7) j = 8;
                    } else {
                        number2 += (list->binary_address[i] - '0') * pow(2, j);
                    }
                    j--;

                }

                list->number1 = number1;
                list->number2 = number2;

                list = list->next;
            }
        } else if (arg2 == 3) { // converte binario inteiro

            while (list != NULL) {
                int number = 0;
                int j = 15;

                for (int i = 0; i < 16; i++) {
                    number += (list->binary_address[i] - '0') * pow(2, j);
                    j--;
                }

                list->binary_to_decimal = number;

                list = list->next;
            }
        }
    }
}

void print_addresses(FILE *output, page *list) {
    if (list == NULL) return;
    
    print_addresses(output, list->next);
    fprintf(output, "Virtual address: %d ", list->virtual_address);
    fprintf(output, "TLB: %d ", list->TLB);
    fprintf(output, "Number1: %d ", list->number1);
    fprintf(output, "Number2: %d \n", list->number2);
}

void fifo(page *list){
    
}

int main() {
    page *list = NULL;
    read_file("addresses.txt", &list);
    convert(list, DECIMAL_TO_BINARY, SEPARATE_FALSE); 
    convert(list, BINARY_TO_DECIMAL, SEPARATE_TRUE);

    FILE *output = fopen("correct2.txt", "w");
    if (output == NULL) {return 1;}

    print_addresses(output, list);
    fclose(output);

    return 0;
}