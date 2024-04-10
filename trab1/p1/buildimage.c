/* Author(s): Samuel de Oliveira Krabbe e Leonardo de Moraes Perin
 * Creates operating system image suitable for placement on a boot disk
*/

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define IMAGE_FILE "./image"
#define BOOT_FILENAME "bootblock"
#define KERNEL_FILENAME "kernel"
#define ARGS "[--extended] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512       /* floppy sector size in bytes */
#define BOOTLOADER_SIG_OFFSET 0x1fe /* offset for boot loader signature */


// Define a struct to hold the package values
typedef struct {
    FILE *imagefile;
    FILE *bootfile;
    FILE *kernelfile;
    Elf32_Ehdr *boot_elf_header;
    Elf32_Phdr *boot_program_header;
    Elf32_Ehdr *kernel_elf_header;
    Elf32_Phdr *kernel_program_header;
    unsigned short num_kernel_sectors;
} Package;


/* Reads elf header from bootblock */
void read_bootblock_ehdr(Package **my_package) {

    // Allocate memory for the ELF header
    (*my_package)->boot_elf_header = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));

    if ((*my_package)->boot_elf_header == NULL) {
        perror("Memory allocation of boot_elf_header failed");
        exit(EXIT_FAILURE);
    }

    // Read the ELF header from the file
    if (fread((*my_package)->boot_elf_header, sizeof(Elf32_Ehdr), 1, (*my_package)->bootfile) != 1) {
        fprintf(stderr, "Error reading ELF header from %s ELF file\n", BOOT_FILENAME);
        free((*my_package)->boot_elf_header);
    }
}

/* Reads elf header from kernel */
void read_kernel_ehdr(Package **my_package) {

    // Allocate memory for the ELF header
    (*my_package)->kernel_elf_header = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));

    if ((*my_package)->kernel_elf_header == NULL) {
        perror("Memory allocation of kernel_elf_header failed");
        exit(EXIT_FAILURE);
    }

    // Read the ELF header from the file
    if (fread((*my_package)->kernel_elf_header, sizeof(Elf32_Ehdr), 1, (*my_package)->kernelfile) != 1) {
        fprintf(stderr, "Error reading ELF header from %s ELF file\n", KERNEL_FILENAME);
        free((*my_package)->kernel_elf_header);
    }
}

/* Reads program header table from bootblock */
void read_bootblock_phdr(Package **my_package) {

    // Allocate memory for the ELF header
    (*my_package)->boot_program_header = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

    if ((*my_package)->boot_program_header == NULL) {
        perror("Memory allocation of boot_program_header failed");
        exit(EXIT_FAILURE);
    }

    // Read program header table
    fseek((*my_package)->bootfile, (*my_package)->boot_elf_header->e_phoff, SEEK_SET);
    fread((*my_package)->boot_program_header, (*my_package)->boot_elf_header->e_phentsize, (*my_package)->boot_elf_header->e_phnum, (*my_package)->bootfile);
}

/* Reads program header table from kernel */
void read_kernel_phdr(Package **my_package) {

    // Allocate memory for the ELF header
    (*my_package)->kernel_program_header = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

    if ((*my_package)->kernel_program_header == NULL) {
        perror("Memory allocation of kernel_program_header failed");
        exit(EXIT_FAILURE);
    }

    // Read program header table
    fseek((*my_package)->kernelfile, (*my_package)->kernel_elf_header->e_phoff, SEEK_SET);
    fread((*my_package)->kernel_program_header, (*my_package)->kernel_elf_header->e_phentsize, (*my_package)->kernel_elf_header->e_phnum, (*my_package)->kernelfile);
}

/* Writes the bootblock to image */
void write_bootblock(Package **my_package) {

    // Allocate memory for bootblock
    unsigned char *bootblock = (unsigned char *)malloc((*my_package)->boot_program_header->p_memsz);
    if (bootblock == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Read bootfile to bootblock
    fseek((*my_package)->bootfile, (*my_package)->boot_program_header->p_offset, SEEK_SET);

    // Read and write bootfile to image
    fread(bootblock, (*my_package)->boot_program_header->p_filesz, 1, (*my_package)->bootfile);
    fwrite(bootblock, (*my_package)->boot_program_header->p_filesz, 1, (*my_package)->imagefile);

    // Write padding to the image
    int bootblock_padding = (SECTOR_SIZE - 2) - (*my_package)->boot_program_header->p_filesz;
    if (bootblock_padding > 0) {
        unsigned char *padding_buffer = (unsigned char *)calloc(bootblock_padding, sizeof(unsigned char));
        if (padding_buffer == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        memset(padding_buffer, 0, bootblock_padding);
        fwrite(padding_buffer, sizeof(unsigned char), bootblock_padding, (*my_package)->imagefile);
        free(padding_buffer);
    }

    // Write the boot signature (aa55) to the last two bytes
    unsigned short boot_signature = 0xaa55;
    fwrite(&boot_signature, sizeof(unsigned short), 1, (*my_package)->imagefile);

    free(bootblock);
}

/* Writes the kernel to image */
void write_kernel(Package **my_package) {

    int i, kernel_padding, last_segment_size, last_sector_padding;
    Elf32_Phdr current_header;
    Elf32_Phdr next_header;

    // Write each segment of program header
    for (i = 0; i < (*my_package)->kernel_elf_header->e_phnum; i++)
    {
        current_header = (*my_package)->kernel_program_header[i];
        next_header = (*my_package)->kernel_program_header[i + 1];

        // Calculate kernel padding
        kernel_padding = next_header.p_offset - (current_header.p_offset + current_header.p_filesz);
        
        if (i == ((*my_package)->kernel_elf_header->e_phnum - 1)) {
            last_segment_size = (*my_package)->kernel_program_header[i].p_filesz;
            last_sector_padding = SECTOR_SIZE - (last_segment_size % SECTOR_SIZE);
            if (last_sector_padding == SECTOR_SIZE) 
                kernel_padding = 0;
            else 
                kernel_padding = last_sector_padding;
        }

        // Allocate memory for the segment
        unsigned char *segment = (unsigned char *)malloc((*my_package)->kernel_program_header[i].p_memsz);
        if (segment == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        // Read segment from kernelfile
        fseek((*my_package)->kernelfile, (*my_package)->kernel_program_header[i].p_offset, SEEK_SET);
        fread(segment, (*my_package)->kernel_program_header[i].p_filesz, 1, (*my_package)->kernelfile);

        // Write segment to the image
        fwrite(segment, (*my_package)->kernel_program_header[i].p_filesz, 1, (*my_package)->imagefile);

        // Write kernel padding to the image
        if (kernel_padding > 0) {
            unsigned char *padding_buffer = (unsigned char *)calloc(kernel_padding, sizeof(unsigned char));
            if (padding_buffer == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
            memset(padding_buffer, 0, kernel_padding);
            fwrite(padding_buffer, sizeof(unsigned char), kernel_padding, (*my_package)->imagefile);
            free(padding_buffer);
        }

        free(segment);
    }
}

/* Counts the number of sectors in the kernel */
void count_kernel_sectors(Package **my_package) {   

    int i;
    unsigned int total_kernel_size = 0;

    // Iterate through all program headers and sum up the sizes of all segments
    for (i = 0; i < (*my_package)->kernel_elf_header->e_phnum; i++) {
        total_kernel_size += (*my_package)->kernel_program_header[i].p_filesz;
    }

    // Calculate the number of sectors required to store the kernel
    (*my_package)->num_kernel_sectors = (unsigned short)((total_kernel_size + SECTOR_SIZE - 1) / SECTOR_SIZE);
}


/* Records the number of sectors in the kernel to 2nd position in image */
void record_kernel_sectors(Package **my_package) {

    // Write os_size into 2nd position of image
    fseek((*my_package)->imagefile, sizeof(unsigned short), SEEK_SET);
    fwrite(&((*my_package)->num_kernel_sectors), 2, 1, (*my_package)->imagefile);
}

// Builds image
void build_image(Package **my_package) {

    if ((*my_package)->imagefile == NULL) {
        (*my_package)->imagefile = fopen("image", "wb");
        if ((*my_package)->imagefile == NULL) {
            perror("Error opening imagefile");
            exit(EXIT_FAILURE);
        }
    }

    // Write bootblock to image
    write_bootblock(my_package);

    // Write kernel to image
    write_kernel(my_package);

    // Record number of kernel sectors in image
    record_kernel_sectors(my_package);
}


/* Prints segment information for --extended option */
void extended_opt(Package *my_package) {

    int bootblock_padding, kernel_padding, last_segment_size, last_sector_padding;
    Elf32_Phdr current_header;
    Elf32_Phdr next_header;
    int i, j;

    // Bootblock segment info
    printf("0x%04x: ./%s\n", my_package->boot_program_header[0].p_vaddr, BOOT_FILENAME);
    for (i = 0; i < my_package->boot_elf_header->e_phnum; ++i) {
        bootblock_padding = SECTOR_SIZE - my_package->boot_program_header->p_filesz;

        printf("  Segment %d:\n", i);
        printf("    offset: 0x%04x\n", my_package->boot_program_header[i].p_offset);
        printf("    vaddr: 0x%04x\n", my_package->boot_program_header[i].p_vaddr);
        printf("    filesz: 0x%04x\n", my_package->boot_program_header[i].p_filesz);
        printf("    memsz: 0x%04x\n", my_package->boot_program_header[i].p_memsz);
        printf("    writing 0x%04x bytes\n", my_package->boot_program_header[i].p_memsz);
        printf("    padding uo to 0x%04x\n", my_package->boot_program_header[i].p_filesz + bootblock_padding);
        printf("\n");
    }

    // Print kernel segment info
    printf("0x%04x: ./%s\n", my_package->kernel_program_header[0].p_vaddr, KERNEL_FILENAME);
    for (j = 0; j < my_package->kernel_elf_header->e_phnum; ++j) {
        current_header = my_package->kernel_program_header[j];
        next_header = my_package->kernel_program_header[j + 1];

        kernel_padding = next_header.p_offset - (current_header.p_offset + current_header.p_filesz);
        if (j == (my_package->kernel_elf_header->e_phnum - 1)) {
            last_segment_size = my_package->kernel_program_header[j].p_filesz;
            last_sector_padding = SECTOR_SIZE - (last_segment_size % SECTOR_SIZE);
            if (last_sector_padding == SECTOR_SIZE) 
                kernel_padding = 0;
            else 
                kernel_padding = last_sector_padding;
        }
        kernel_padding += my_package->boot_program_header[i].p_filesz + bootblock_padding + my_package->kernel_program_header[j].p_filesz;

        printf("  Segment %d:\n", j);
        printf("    offset: 0x%04x\n", my_package->kernel_program_header[j].p_offset);
        printf("    vaddr: 0x%04x\n", my_package->kernel_program_header[j].p_vaddr);
        printf("    filesz: 0x%04x\n", my_package->kernel_program_header[j].p_filesz);
        printf("    memsz: 0x%04x\n", my_package->kernel_program_header[j].p_memsz);
        printf("    writing 0x%04x bytes\n", my_package->kernel_program_header[j].p_memsz);
        printf("    padding uo to 0x%04x\n", kernel_padding);
        printf("\n");
    }

    // Print os_size
    printf("os_size: %d\n", my_package->num_kernel_sectors);
    printf("\n");
}

/* MAIN */
int main(int argc, char **argv)
{
    // Package structure that holds all the values I will need
    Package *my_package = malloc(sizeof(Package));
    if (my_package == NULL) {
        perror("Error allocating memory for my_package");
        exit(EXIT_FAILURE);
    }

    // Opens bootfile
    my_package->bootfile = fopen(BOOT_FILENAME, "rb");
    if (my_package->bootfile == NULL) {
        printf("Error opening %s\n", BOOT_FILENAME);
        exit(EXIT_FAILURE);
    }

    // Opens kernelfile
    my_package->kernelfile = fopen(KERNEL_FILENAME, "rb");
    if (my_package->kernelfile == NULL) {
        printf("Error opening %s\n", KERNEL_FILENAME);
        exit(EXIT_FAILURE);
    }

    // Read ELF files' ELF headers
    read_bootblock_ehdr(&my_package);
    read_kernel_ehdr(&my_package);

	/* read ELF files' program headers */  
    read_bootblock_phdr(&my_package);
    read_kernel_phdr(&my_package);

    /* Counts the number of sectors in kernelfile */
    count_kernel_sectors(&my_package);

	/* builds image*/
    build_image(&my_package);

	/* check for --extended option */
    if (argc > 1) {
        if (!strncmp(argv[1], "--extended", 11)) {
            extended_opt(my_package);
        } else {
            // Case where the argument is not "--extended"
            printf("\n");
            printf("Unknown option. Usage: ./program --extended\n");
            printf("\n");
        }
    } else {
        // Case where no command line argument is provided
        printf("\n");
        printf("Usage: %s\n", ARGS);
        printf("\n");
    }
	
	// Cleaning pointers and closing files
    free(my_package->boot_elf_header);
    free(my_package->kernel_elf_header);
    free(my_package->boot_program_header);
    free(my_package->kernel_program_header);
    fclose(my_package->bootfile);
    fclose(my_package->kernelfile);
	fclose(my_package->imagefile);
  
	return 0;
}
