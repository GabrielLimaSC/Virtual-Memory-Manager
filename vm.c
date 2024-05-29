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
#define FIFO_ARG 9
#define LRU_ARG 10

typedef struct page
{
    int virtual_address;
    char binary_address[17];
    int page_number;
    int offset;
    int frame_number;
    int binary_to_decimal;
    int TLB;
    int TLB_address;
    int translated_address;
    int TLB_Hits;
    int page_faults;
    int instruction;
    int physical_address;
    int value;
    int count;
    bool is_valid; // Novo campo para indicar se o frame é válido
    struct page *next;
    struct page *prev;
} page;

void read_file(char *filename, page **list);
void convert(page *list, int arg, int arg2);
void print_addresses(FILE *output, page *list, int arg);
void search_instruction(page *list, FILE *file);
int signed_char_to_int(unsigned char byte);
void fifo(page *list, int arg, int arg2);
void lru(page *list, int arg, int arg2);

void read_file(char *filename, page **list)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        exit(1);
    }

    char line[20];
    int translated = 0;
    page *tail = NULL;

    while (fgets(line, sizeof(line), file))
    {
        page *new_page = (page *)malloc(sizeof(page)); // Usando malloc para alocar memória
        if (new_page == NULL)
        {
            exit(1);
        }
        memset(new_page, 0, sizeof(page)); // Inicializa a memória com zeros
        new_page->virtual_address = atoi(line);
        new_page->frame_number = -1;
        new_page->count = 1;
        new_page->is_valid = true; // Inicializa como válido
        translated++;
        new_page->next = NULL;
        new_page->prev = tail;

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
    if (*list != NULL)
    {
        (*list)->translated_address = translated;
    }

    fclose(file);
}

void convert(page *list, int arg, int arg2)
{
    if (arg == DECIMAL_TO_BINARY)
    { 
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
    {
        if (arg2 == SEPARATE_TRUE)
        {
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
        {
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

void fifo(page *list, int arg, int arg2)
{
    static page *temp_list = NULL;
    static int tlb_index = 0;
    static int frame_index = 0;
    static int initialized_tlb = 0; // Número de entradas inicializadas no TLB
    static int initialized_frames = 0; // Número de entradas inicializadas nos frames físicos
    int tlb_hits = 0;
    int page_faults = 0;

    if (arg == TLB_TRUE)
    {
        if (list == NULL)
            return;

        page *current = list;

        if (temp_list == NULL)
        {
            temp_list = (page *)malloc(sizeof(page) * 16); // Aloca memória usando malloc
            memset(temp_list, 0, sizeof(page) * 16); // Inicializa a memória com zeros
        }

        while (current != NULL)
        {
            bool found = false;

            for (int i = 0; i < initialized_tlb; i++)
            {
                if (temp_list[i].page_number == current->page_number)
                {
                    current->TLB = temp_list[i].TLB;
                    current->value = temp_list[i].value;
                    tlb_hits++;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                current->TLB = tlb_index;
                temp_list[tlb_index].page_number = current->page_number;
                temp_list[tlb_index].TLB = current->TLB;
                tlb_index = (tlb_index + 1) % 16;
                if (initialized_tlb < 16) initialized_tlb++;
            }
            current->TLB_Hits = tlb_hits;

            current = current->next;
        }
    }

    static page *temp_list2 = NULL;

    if (arg2 == PHYSICAL_MEMORY_TRUE)
    {
        if (list == NULL)
            return;

        page *current2 = list;

        if (temp_list2 == NULL)
        {
            temp_list2 = (page *)malloc(sizeof(page) * 128); // Aloca memória usando malloc
            memset(temp_list2, 0, sizeof(page) * 128); // Inicializa a memória com zeros
        }

        while (current2 != NULL)
        {
            bool found = false;

            for (int i = 0; i < initialized_frames; i++)
            {
                if (temp_list2[i].page_number == current2->page_number)
                {
                    int frame_indexx = temp_list2[i].frame_number;
                    current2->frame_number = frame_indexx;
                    current2->value = temp_list2[i].value;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                current2->frame_number = frame_index;
                temp_list2[frame_index].page_number = current2->page_number;
                temp_list2[frame_index].frame_number = frame_index; // Atribui corretamente o número do frame
                temp_list2[frame_index].virtual_address = current2->virtual_address;

                frame_index = (frame_index + 1) % 128;
                if (initialized_frames < 128) initialized_frames++;
                page_faults++;
            }

            // Verificação e invalidação de duplicatas
            page *iterator = list;
            while (iterator != NULL) {
                if (iterator != current2 && iterator->frame_number == current2->frame_number) {
                    iterator->is_valid = false; // Marca como inválido
                }
                iterator = iterator->next;
            }

            current2 = current2->next;
        }
        list->page_faults = page_faults;
    }
}

void lru(page *list, int arg, int arg2) {
    static page *temp_list = NULL;
    static int frame_index = 0;
    static int initialized_tlb = 0; // Número de entradas inicializadas no TLB
    static int initialized_frames = 0; // Número de entradas inicializadas nos frames físicos
    int tlb_hits = 0;
    int page_faults = 0;

    if (arg == TLB_TRUE) {
        if (list == NULL)
            return;

        page *current = list;

        if (temp_list == NULL) {
            temp_list = (page *)malloc(sizeof(page) * 16); // Aloca memória usando malloc
            memset(temp_list, 0, sizeof(page) * 16); // Inicializa a memória com zeros
        }

        while (current != NULL) {
            bool found = false;

            for (int i = 0; i < initialized_tlb; i++) {
                if (temp_list[i].page_number == current->page_number) {
                    current->TLB = temp_list[i].TLB;
                    current->value = temp_list[i].value;
                    tlb_hits++;
                    found = true;

                    // Usada recentemente, mover para o final
                    page temp = temp_list[i];
                    for (int j = i; j < initialized_tlb - 1; j++) {
                        temp_list[j] = temp_list[j + 1];
                    }
                    temp_list[initialized_tlb - 1] = temp;
                    break;
                }
            }

            if (!found) {
                for (int i = 0; i < initialized_tlb - 1; i++) {
                    temp_list[i] = temp_list[i + 1];
                }
                temp_list[initialized_tlb - 1] = *current;
                current->TLB = initialized_tlb - 1;
                if (initialized_tlb < 16) initialized_tlb++;
            }
            current->TLB_Hits = tlb_hits;

            current = current->next;
        }
    }

    static page *temp_list2 = NULL;

    if (arg2 == PHYSICAL_MEMORY_TRUE) {
        if (list == NULL)
            return;

        page *current2 = list;

        if (temp_list2 == NULL) {
            temp_list2 = (page *)malloc(sizeof(page) * 128); // Aloca memória usando malloc
            memset(temp_list2, 0, sizeof(page) * 128); // Inicializa a memória com zeros
        }

        while (current2 != NULL) {
            bool found = false;

            for (int i = 0; i < initialized_frames; i++) {
                if (temp_list2[i].page_number == current2->page_number) {
                    current2->frame_number = temp_list2[i].frame_number;
                    current2->value = temp_list2[i].value;
                    found = true;

                    // Usada recentemente, mover para o final
                    page temp = temp_list2[i];
                    for (int j = i; j < initialized_frames - 1; j++) {
                        temp_list[j] = temp_list[j + 1];
                    }
                    temp_list[initialized_frames - 1] = temp;
                    break;
                }
            }

            if (!found) {
                current2->frame_number = frame_index;
                temp_list2[frame_index].page_number = current2->page_number;
                temp_list2[frame_index].frame_number = frame_index; // Atribui corretamente o número do frame
                temp_list2[frame_index].virtual_address = current2->virtual_address;

                frame_index = (frame_index + 1) % 128;
                if (initialized_frames < 128) initialized_frames++;
                page_faults++;
            }

            // Verificação e invalidação de duplicatas
            page *iterator = list;
            while (iterator != NULL) {
                if (iterator != current2 && iterator->frame_number == current2->frame_number) {
                    iterator->is_valid = false; // Marca como inválido
                }
                iterator = iterator->next;
            }

            current2 = current2->next;
        }
        list->page_faults = page_faults;
    }
}

void search_instruction(page *list, FILE *file)
{
    page *head = list; // To iterate from the head of the list

    while (list != NULL)
    {
        // Iterate to find the page with the same page_number and update frame_number if needed
        

        unsigned long long file_position = (unsigned long long)list->page_number * PAGE_BYTE_SIZE + list->offset;
        fseek(file, file_position, SEEK_SET);

        unsigned char byte;
        fread(&byte, 1, 1, file);

        unsigned char masked_byte = byte & 0xFF;

        int value = signed_char_to_int(masked_byte);

        list->value = value;
        list->physical_address = list->offset + (list->frame_number * PAGE_BYTE_SIZE);

        page *current = head;
        while (current != NULL)
        {
            if (current->page_number == list->page_number && current->is_valid == true)
            {
                
                if (list->virtual_address == 50226){
                    printf("%d\n", list->virtual_address);
                    printf("%d\n", current->virtual_address);
                    printf("%d\n", current->is_valid);
                }
                list->frame_number = current->frame_number;
                break;
            }
            current = current->next;
        }

        list = list->next;
    }
}


void print_addresses(FILE *output, page *list, int arg)
{
    page *current = list;
    if (arg == FIFO_ARG)
    {
        while (current != NULL)
        {
            fprintf(output, "Virtual address: %d ", current->virtual_address);
            // fprintf(output, "Binary Number: %s ", current->binary_address);
            // fprintf(output, "Page Number: %d ", current->page_number);
            // fprintf(output, "Offset: %d ", current->offset);
            // fprintf(output, "Frame Number: %d ", current->frame_number);
            fprintf(output, "TLB: %d ", current->TLB);
            fprintf(output, "Physical address: %d ", current->physical_address);
            fprintf(output, "Value: %d\n", current->value);
            list->TLB_Hits = current->TLB_Hits;
            current = current->next;
        }
        fprintf(output, "Number of Translated Addresses = %d\n", list->translated_address);
        fprintf(output, "Page Faults = %d\n", list->page_faults);
        fprintf(output, "Page Fault Rate = %.3f\n", (float)list->page_faults / (float)list->translated_address);
        fprintf(output, "TLB Hits = %d\n", list->TLB_Hits);
        fprintf(output, "TLB Hit Rate = %.3f\n", (float)list->TLB_Hits / (float)list->translated_address);
    }

    if (arg == LRU_ARG)
    {
        while (current != NULL)
        {
            fprintf(output, "Virtual address: %d ", current->virtual_address);
            // fprintf(output, "Binary Number: %s ", current->binary_address);
            // fprintf(output, "Page Number: %d ", current->page_number);
            // fprintf(output, "Offset: %d ", current->offset);
            // fprintf(output, "Frame Number: %d ", current->frame_number);
            fprintf(output, "TLB: %d ", current->TLB);
            fprintf(output, "Physical address: %d ", current->physical_address);
            fprintf(output, "Value: %d\n", current->value);
            list->TLB_Hits = current->TLB_Hits;
            current = current->next;
        }
        fprintf(output, "Number of Translated Addresses = %d\n", list->translated_address);
        fprintf(output, "Page Faults = %d\n", list->page_faults);
        fprintf(output, "Page Fault Rate = %.3f\n", (float)list->page_faults / (float)list->translated_address);
        fprintf(output, "TLB Hits = %d\n", list->TLB_Hits);
        fprintf(output, "TLB Hit Rate = %.3f\n", (float)list->TLB_Hits / (float)list->translated_address);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "%s <file> <algorithm>\n", argv[0]);
        return 1;
    }

    char *address_file = argv[1];
    char *algorithm = argv[2];

    page *list = NULL;
    read_file(address_file, &list);
    convert(list, DECIMAL_TO_BINARY, SEPARATE_FALSE);
    convert(list, BINARY_TO_DECIMAL, SEPARATE_TRUE);
    fifo(list, TLB_TRUE, PHYSICAL_MEMORY_FALSE); // TLB será aplicado apenas no FIFO

    if (strcmp(algorithm, "fifo") == 0)
    {
        fifo(list, TLB_FALSE, PHYSICAL_MEMORY_TRUE);
    }
    if (strcmp(algorithm, "lru") == 0)
    {
        lru(list, TLB_FALSE, PHYSICAL_MEMORY_TRUE);
    }

    FILE *file = fopen("BACKING_STORE.bin", "rb");
    if (file == NULL)
    {
        return 1;
    }

    search_instruction(list, file);

    FILE *output = fopen("correct2.txt", "w");
    if (output == NULL)
    {
        return 1;
    }

    if (strcmp(algorithm, "fifo") == 0)
    {
        print_addresses(output, list, FIFO_ARG);
    }
    if (strcmp(algorithm, "lru") == 0)
    {
        print_addresses(output, list, LRU_ARG);
    }

    fclose(output);

    return 0;
}
