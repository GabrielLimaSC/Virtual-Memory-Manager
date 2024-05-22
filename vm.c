#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define PAGE_BYTE_SIZE 256
#define MAX_ADDRESS 65536
#define DECIMAL_TO_BINARY 1
#define BINARY_TO_DECIMAL 2
#define SEPARATE_FALSE 3
#define SEPARATE_TRUE 4
#define TLB_TRUE 5
#define TLB_FALSE 6
#define PHYSICAL_MEMORY_TRUE 7
#define PHYSICAL_MEMORY_FALSE 8

typedef struct page
{
    int virtual_address;
    int binary_address[17];
    int page_number;
    int offset;
    int frame_number;
    int binary_to_decimal;
    int TLB;
    int TLB_address;
    int instruction;
    int physical_address;
    int value;
    struct page *next;

} page;

void read_file(char *filename, page **list);
void convert(page *list, int arg, int arg2);
void print_addresses(FILE *output, page *list);
void search_instruction(page *list, FILE *file);
int signed_char_to_int(unsigned char byte);
void fifo(page *list, int arg, int arg2);

void read_file(char *filename, page **list)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        exit(1);
    }

    char line[20];
    page *tail = NULL;

    while (fgets(line, sizeof(line), file))
    {
        page *new_page = (page *)malloc(sizeof(page));
        if (new_page == NULL)
        {
            exit(1);
        }

        new_page->virtual_address = atoi(line);
        new_page->next = NULL;

        if (*list == NULL)
        {
            *list = new_page;
        }
        else
        {
            tail->next = new_page;
        }

        tail = new_page;
    }

    fclose(file);
}

void convert(page *list, int arg, int arg2)
{
    if (arg == DECIMAL_TO_BINARY)
    { // converte de decimal para binário
        while (list != NULL)
        {
            list->binary_address[16] = '\0';
            int n = list->virtual_address;

            for (int i = 15; i >= 0; i--)
            {
                list->binary_address[i] = (n % 2) + '0';
                n /= 2;
            }

            list = list->next;
        }
    }
    else if (arg == BINARY_TO_DECIMAL)
    { // converte de binário para decimal

        if (arg2 == SEPARATE_TRUE)
        { // separa binario em dois numeros e converte para decimal
            while (list != NULL)
            {
                int page_number = 0;
                int offset = 0;
                int j = 7;

                for (int i = 0; i < 16; i++)
                {
                    if (i < 8)
                    {
                        page_number += (list->binary_address[i] - '0') * pow(2, j);
                        if (i == 7)
                            j = 8;
                    }
                    else
                    {
                        offset += (list->binary_address[i] - '0') * pow(2, j);
                    }
                    j--;
                }

                list->page_number = page_number;
                list->offset = offset;

                list = list->next;
            }
        }
        else if (arg2 == SEPARATE_FALSE)
        { // converte binario inteiro
            while (list != NULL)
            {
                int number = 0;
                int j = 15;

                for (int i = 0; i < 16; i++)
                {
                    number += (list->binary_address[i] - '0') * pow(2, j);
                    j--;
                }

                list->binary_to_decimal = number;

                list = list->next;
            }
        }
    }
}

int signed_char_to_int(unsigned char byte)
{
    if ((byte & 0x80) == 0x80)
    {
        byte = ~byte;
        byte += 1;
        return -((int)byte);
    }
    else
    {
        return (int)byte;
    }
}

void fifo(page *list, int arg, int arg2) // arg = tlb, arg2 = frame_number
{
    static page *temp_list = NULL; 

    if (arg == TLB_TRUE) // FIFO para TLB
    {
        if (list == NULL)
            return;

        int tlb_index = 0;
        page *current = list;
        
        if (temp_list == NULL)
        {
            temp_list = (page *)malloc(sizeof(page) * 16);
            memset(temp_list, 0, sizeof(page) * 16);
        }

        while (current != NULL)
        {
            bool found = false;

            for (int i = 0; i < 16; i++)
            {
                if (temp_list[i].page_number == current->page_number)
                {
                    current->TLB = temp_list[i].TLB;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                current->TLB = tlb_index;
                tlb_index = (tlb_index + 1) % 16;
            }

            temp_list[tlb_index].page_number = current->page_number;
            temp_list[tlb_index].TLB = current->TLB;

            current = current->next;
        }
    }

    if (arg2 == PHYSICAL_MEMORY_TRUE) // FIFO para frame_number
    {
        if (list == NULL)
            return;

        int frame_index = 0;
        page *current = list;

        while (current != NULL)
        {
            current->frame_number = frame_index;
            frame_index = (frame_index + 1) % 256; // Incrementa e reinicia ao atingir 256
            current = current->next;
        }
    }
}

void search_instruction(page *list, FILE *file)
{
    while (list != NULL)
    {
        unsigned long long file_position = (unsigned long long)list->page_number * PAGE_BYTE_SIZE + list->offset;
        fseek(file, file_position, SEEK_SET);

        unsigned char byte;
        fread(&byte, 1, 1, file);

        unsigned char masked_byte = byte & 0xFF;

        int value = signed_char_to_int(masked_byte);

        list->value = value;
        list->physical_address = list->offset + (list->frame_number * PAGE_BYTE_SIZE);

        list = list->next;
    }
}

void print_addresses(FILE *output, page *list)
{
    page *current = list;
    while (current != NULL)
    {
        fprintf(output, "Virtual address: %d ", current->virtual_address);
        fprintf(output, "TLB: %d ", current->TLB);
        fprintf(output, "Physical address: %d ", current->physical_address);
        fprintf(output, "value: %d\n", current->value);
        current = current->next;
    }
}

int main()
{
    page *list = NULL;
    read_file("addresses.txt", &list);
    convert(list, DECIMAL_TO_BINARY, SEPARATE_FALSE);
    convert(list, BINARY_TO_DECIMAL, SEPARATE_TRUE);
    fifo(list, TLB_TRUE, PHYSICAL_MEMORY_FALSE);
    fifo(list, TLB_FALSE, PHYSICAL_MEMORY_TRUE);
    FILE *file = fopen("BACKING_STORE.bin", "rb");
    search_instruction(list, file);

    FILE *output = fopen("correct2.txt", "w");
    if (output == NULL)
    {
        return 1;
    }

    print_addresses(output, list);
    fclose(output);

    return 0;
}
