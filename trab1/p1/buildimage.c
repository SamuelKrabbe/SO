/* Author(s): Samuel de Oliveira Krabbe
 * Creates operating system image suitable for placement on a boot disk
*/

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define BOOT_FILENAME "bootblock"
#define KERNEL_FILENAME "kernel"
#define ARGS "[--extended] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512       /* floppy sector size in bytes */
#define BOOTLOADER_SIG_OFFSET 0x1fe /* offset for boot loader signature */

// PROGRAM HEADER TYPES
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6
#define PT_LOPROC   0x70000000
#define PT_HIPROC   0x7fffffff


// Define a struct to hold the package values
typedef struct {
    FILE *imagefile;
    FILE *bootfile;
    FILE *kernelfile;
    Elf32_Ehdr *boot_elf_header;
    Elf32_Phdr *boot_program_header;
    Elf32_Ehdr *kernel_elf_header;
    Elf32_Phdr *kernel_program_header;
    int num_bootblock_sectors;
    int num_kernel_sectors;
} Package;


// Function to read ELF header from bootblock
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

// Function to read ELF header from kernel
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

/* Writes the bootblock to the image file */
void write_bootblock(Package **my_package)
{
    // Allocate memory for bootblock
    unsigned char *bootblock = (unsigned char *)malloc((*my_package)->boot_program_header->p_filesz);
    if (bootblock == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Read the bootblock from the bootfile
    fseek((*my_package)->bootfile, (*my_package)->boot_program_header->p_offset, SEEK_SET);
    fread(bootblock, 1, (*my_package)->boot_program_header->p_filesz, (*my_package)->bootfile);

    // Write the bootblock to the image file
    fwrite(bootblock, 1, (*my_package)->boot_program_header->p_filesz, (*my_package)->imagefile);

    // Free allocated memory
    free(bootblock);
}

/* Writes the kernel to the image file */
void write_kernel(Package **my_package)
{ 
    // Allocate memory for kernel
    unsigned char *kernel = (unsigned char *)malloc((*my_package)->kernel_program_header->p_filesz);
    if (kernel == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Read the kernel from the kernelfile
    fseek((*my_package)->kernelfile, (*my_package)->kernel_program_header->p_offset, SEEK_SET);
    fread(kernel, 1, (*my_package)->kernel_program_header->p_filesz, (*my_package)->kernelfile);

    // Write the kernel to the image file
    fwrite(kernel, 1, (*my_package)->kernel_program_header->p_filesz, (*my_package)->imagefile);

    // Free allocated memory
    free(kernel);
}

/* Counts the number of sectors in the kernel */
void count_kernel_sectors(Package **my_package) {   
    // Calculate the size of the kernel in bytes
    unsigned int kernel_size = (*my_package)->kernel_program_header->p_filesz;

    // Calculate the number of sectors required to store the kernel
    (*my_package)->num_kernel_sectors = (kernel_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
}

void count_bootblock_sectors(Package **my_package) {
    // Calculate the size of the bootblock in bytes
    unsigned int bootblock_size = (*my_package)->boot_program_header->p_filesz;

    // Calculate the number of sectors required to store the bootblock
    (*my_package)->num_bootblock_sectors = (bootblock_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
}

/* Records the number of sectors in the kernel */
void record_kernel_sectors(Package **my_package) {
    // Write the number of sectors to the image file
    fwrite((&(*my_package)->num_kernel_sectors), sizeof(int), 1, (*my_package)->imagefile);
}

// Build image file
void build_image(Package **my_package) {
    if ((*my_package)->imagefile == NULL) {
        (*my_package)->imagefile = fopen("image", "wb");
        if ((*my_package)->imagefile == NULL) {
            perror("Error opening image file");
            exit(EXIT_FAILURE);
        }
    }

    // Write bootblock to the image file
    write_bootblock(my_package);

    // Write kernel to the image file
    write_kernel(my_package);

    // Record number of kernel sectors in the image file
    record_kernel_sectors(my_package);
}

/* Find the corresponding string representation for header type */
void getHeaderType(Package *my_package, int current_hdr_index) {
    int k;

    // Array to store string representations and numeric values of program header types
    const char *program_header_types[] = {
        "PT_NULL", "PT_LOAD", "PT_DYNAMIC", "PT_INTERP", "PT_NOTE", "PT_SHLIB", "PT_PHDR",
        "PT_LOPROC", "PT_HIPROC"
    };
    int program_header_values[] = {
        PT_NULL, PT_LOAD, PT_DYNAMIC, PT_INTERP, PT_NOTE, PT_SHLIB, PT_PHDR, PT_LOPROC, PT_HIPROC
    };

    int num_program_header_types = sizeof(program_header_values) / sizeof(program_header_values[0]);
    
    const char *type_str = NULL;
    for (k = 0; k < num_program_header_types; ++k) {
        if (my_package->boot_program_header[current_hdr_index].p_type == program_header_values[k]) {
            type_str = program_header_types[k];
            break;
        }
    }
    if (type_str != NULL) {
        printf("    Type: %s\n", type_str);
    } else {
        printf("    Type: Unknown\n");
    }
}


/* Prints segment information for --extended option */
void extended_opt(Package *my_package) {
    int i, j;

    // Calculate total size of the image file in bytes
    fseek(my_package->imagefile, 0, SEEK_END);
    long total_size = ftell(my_package->imagefile);

    // Calculate total number of sectors used by the image
    int total_sectors = (total_size + SECTOR_SIZE - 1) / SECTOR_SIZE;

    // Print number of disk sectors used by the image
    printf("\n");
    printf("Number of disk sectors used by the image: %d\n", total_sectors);
    printf("\n");

    // Bootblock segment info
    printf("0x%x: %s\n", my_package->boot_program_header[0].p_vaddr, BOOT_FILENAME);
    for (i = 0; i < my_package->boot_elf_header->e_phnum; ++i) {
        printf("  Segment %d:\n", i + 1);
        getHeaderType(my_package, i);
        printf("    Offset: 0x%x\n", my_package->boot_program_header[i].p_offset);
        printf("    Virtual Address: 0x%x\n", my_package->boot_program_header[i].p_vaddr);
        printf("    Physical Address: 0x%x\n", my_package->boot_program_header[i].p_paddr);
        printf("    Size in file: 0x%x\n", my_package->boot_program_header[i].p_filesz);
        printf("    Size in memory: 0x%x\n", my_package->boot_program_header[i].p_memsz);
        printf("    Flags: 0x%x\n", my_package->boot_program_header[i].p_flags);
        printf("    Alignment: %d bytes\n", my_package->boot_program_header[i].p_align);
        printf("\n");
    }

    // Print number of bootblock sectors
    printf("bootblock size in sectors: %d\n", my_package->num_bootblock_sectors);
    printf("\n");

    // Print kernel segment info
    printf("0x%x: %s\n", my_package->kernel_program_header[0].p_vaddr, KERNEL_FILENAME);
    for (j = 0; j < my_package->kernel_elf_header->e_phnum; ++j) {
        printf("  Segment %d:\n", j + 1);
        getHeaderType(my_package, j);
        printf("    Offset: 0x%x\n", my_package->kernel_program_header[j].p_offset);
        printf("    Virtual Address: 0x%x\n", my_package->kernel_program_header[j].p_vaddr);
        printf("    Physical Address: 0x%x\n", my_package->kernel_program_header[j].p_paddr);
        printf("    Size in file: 0x%x\n", my_package->kernel_program_header[j].p_filesz);
        printf("    Size in memory: 0x%x\n", my_package->kernel_program_header[j].p_memsz);
        printf("    Flags: 0x%x\n", my_package->kernel_program_header[j].p_flags);
        printf("    Alignment: %d bytes\n", my_package->kernel_program_header[j].p_align);
        printf("\n");
    }

    // Print number of kernel sectors
    printf("Kernel size in sectors: %d\n", my_package->num_kernel_sectors);
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

    my_package->bootfile = fopen(BOOT_FILENAME, "rb");
    if (my_package->bootfile == NULL) {
        printf("Error opening %s\n", BOOT_FILENAME);
        exit(EXIT_FAILURE);
    }

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

    /* Counts the number of sectors in the ELF files */
    count_bootblock_sectors(&my_package);
    count_kernel_sectors(&my_package);

	/* build image file */
    build_image(&my_package);

	/* check for --extended option */
    if (argc > 1) {
        if (!strncmp(argv[1], "--extended", 11)) {
            extended_opt(my_package);
        } else {
            // Handle case where the argument is not "--extended"
            printf("\n");
            printf("Unknown option. Usage: ./program --extended\n");
            printf("\n");
        }
    } else {
        // Handle case where no command line argument is provided
        printf("\n");
        printf("Usage: %s\n", ARGS);
        printf("\n");
    }
	
	// Cleaning up
    free(my_package->boot_elf_header);
    free(my_package->kernel_elf_header);
    free(my_package->boot_program_header);
    free(my_package->kernel_program_header);
    fclose(my_package->bootfile);
    fclose(my_package->kernelfile);
	fclose(my_package->imagefile);
  
	return 0;
}
