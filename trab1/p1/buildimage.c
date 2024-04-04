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
        return NULL;
    }

    // Read the ELF header from the file
    if (fread((*my_package)->boot_elf_header, sizeof(Elf32_Ehdr), 1, (*my_package)->bootfile) != 1) {
        fprintf(stderr, "Error reading ELF header from %s ELF file\n", BOOT_FILENAME);
        free((*my_package)->boot_elf_header);
        return NULL;
    }

    printf("%s's ELF header read successfully!\n", BOOT_FILENAME);
}

// Function to read ELF header from kernel
void read_kernel_ehdr(Package **my_package) {

    // Allocate memory for the ELF header
    (*my_package)->kernel_elf_header = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));

    if ((*my_package)->kernel_elf_header == NULL) {
        perror("Memory allocation of kernel_elf_header failed");
        return NULL;
    }

    // Read the ELF header from the file
    if (fread((*my_package)->kernel_elf_header, sizeof(Elf32_Ehdr), 1, (*my_package)->kernelfile) != 1) {
        fprintf(stderr, "Error reading ELF header from %s ELF file\n", KERNEL_FILENAME);
        free((*my_package)->kernel_elf_header);
        return NULL;
    }

    printf("%s's ELF header read successfully!\n", KERNEL_FILENAME);
}

void read_bootblock_phdr(Package **my_package) {

    // Allocate memory for the ELF header
    (*my_package)->boot_program_header = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

    if ((*my_package)->boot_program_header == NULL) {
        perror("Memory allocation of boot_program_header failed");
        return NULL;
    }

    // Read program header table
    fseek((*my_package)->bootfile, (*my_package)->boot_elf_header->e_phoff, SEEK_SET);
    fread((*my_package)->boot_program_header, (*my_package)->boot_elf_header->e_phentsize, (*my_package)->boot_elf_header->e_phnum, (*my_package)->bootfile);

    printf("%s's program header table read successfully!\n", BOOT_FILENAME);
}

void read_kernel_phdr(Package **my_package) {

    // Allocate memory for the ELF header
    (*my_package)->kernel_program_header = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

    if ((*my_package)->kernel_program_header == NULL) {
        perror("Memory allocation of kernel_program_header failed");
        return NULL;
    }

    // Read program header table
    fseek((*my_package)->kernelfile, (*my_package)->kernel_elf_header->e_phoff, SEEK_SET);
    fread((*my_package)->kernel_program_header, (*my_package)->kernel_elf_header->e_phentsize, (*my_package)->kernel_elf_header->e_phnum, (*my_package)->kernelfile);

    printf("%s's program header table read successfully!\n", KERNEL_FILENAME);
}

/* Writes the bootblock to the image file */
void write_bootblock(Package **my_package)
{
    // Allocate memory for bootblock
    unsigned char *bootblock = (unsigned char *)malloc((*my_package)->boot_program_header->p_filesz);
    if (bootblock == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    // Read the bootblock from the bootfile
    fseek((*my_package)->bootfile, (*my_package)->boot_program_header->p_offset, SEEK_SET);
    fread(bootblock, 1, (*my_package)->boot_program_header->p_filesz, (*my_package)->bootfile);

    // Write the bootblock to the image file
    fwrite(bootblock, 1, (*my_package)->boot_program_header->p_filesz, (*my_package)->imagefile);

    // Free allocated memory
    free(bootblock);

    printf("%s written successfully into image!\n", BOOT_FILENAME);
}

/* Writes the kernel to the image file */
void write_kernel(Package **my_package)
{ 
    // Allocate memory for kernel
    unsigned char *kernel = (unsigned char *)malloc((*my_package)->kernel_program_header->p_filesz);
    if (kernel == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    // Read the kernel from the kernelfile
    fseek((*my_package)->kernelfile, (*my_package)->kernel_program_header->p_offset, SEEK_SET);
    fread(kernel, 1, (*my_package)->kernel_program_header->p_filesz, (*my_package)->kernelfile);

    // Write the kernel to the image file
    fwrite(kernel, 1, (*my_package)->kernel_program_header->p_filesz, (*my_package)->imagefile);

    // Free allocated memory
    free(kernel);

    printf("%s written successfully into image!\n", KERNEL_FILENAME);
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
    unsigned int bootblock_size = (*my_package)->bootblock_program_header->p_filesz;

    // Calculate the number of sectors required to store the bootblock
    (*my_package)->num_bootblock_sectors = (bootblock_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
}

/* Records the number of sectors in the kernel */
void record_kernel_sectors(Package **my_package) {
    // Write the number of sectors to the image file
    fwrite((*my_package)->num_kernel_sectors, sizeof(int), 1, (*my_package)->imagefile);
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


/* Prints segment information for --extended option */
void extended_opt(Package *my_package)
{
    // Calculate total size of the image file in bytes
    fseek(imagefile, 0, SEEK_END);
    long total_size = ftell(imagefile);

    // Calculate total number of sectors used by the image
    int total_sectors = (total_size + 511) / 512;

    // Print number of disk sectors used by the image
    printf("Number of disk sectors used by the image: %d\n", total_sectors);

    // Bootblock segment info
    printf("Bootblock segment info:\n");
    for (int i = 0; i < boot_ehdr->e_phnum; ++i) {
        printf("  Segment %d:\n", i + 1);
        printf("  Type: %d\n", boot_phdr[i].p_type);
        printf("  Offset: 0x%x\n", boot_phdr[i].p_offset);
        printf("  Size: %d bytes\n", boot_phdr[i].p_filesz);
        printf("  Virtual Address: 0x%x\n", boot_phdr[i].p_vaddr);
        printf("  Physical Address: 0x%x\n", boot_phdr[i].p_paddr);
    }

    // Print kernel segment info
    printf("Kernel segment info:\n");
    for (int i = 0; i < kernel_ehdr->e_phnum; ++i) {
        printf("  Segment %d:\n", i + 1);
        printf("    Type: %d\n", kernel_phdr[i].p_type);
        printf("    Offset: 0x%x\n", kernel_phdr[i].p_offset);
        printf("    Size: %d bytes\n", kernel_phdr[i].p_filesz);
        printf("    Virtual Address: 0x%x\n", kernel_phdr[i].p_vaddr);
        printf("    Physical Address: 0x%x\n", kernel_phdr[i].p_paddr);
    }

    // Calculate the position of the number of kernel sectors within the file
    long kernel_sectors_pos = total_size - sizeof(int);

    // Seek to the position of the number of kernel sectors within the file
    fseek(imagefile, kernel_sectors_pos, SEEK_SET);

    // Read the number of kernel sectors from the file
    int num_kernel_sectors;
    fread(&num_kernel_sectors, sizeof(int), 1, imagefile);

    // Print number of kernel sectors
    printf("Kernel size in sectors: %d\n", num_kernel_sectors);
}

/* MAIN */
int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s\n", ARGS);
        printf("\n");
    }

    // Package structure that holds all the values I will need
    Package *my_package;

    (*my_package)->bootfile = fopen(BOOT_FILENAME, "rb");
    if ((*my_package)->bootfile == NULL) {
        perror("Error opening bootfile");
        return NULL;
    }

    (*my_package)->kernelfile = fopen(KERNEL_FILENAME, "rb");
    if ((*my_package)->kernelfile == NULL) {
        perror("Error opening kernelfile");
        return NULL;
    }

    // Read ELF files' ELF headers
    my_package->boot_elf_header = read_bootblock_ehdr(&my_package);
    my_package->kernel_elf_header = read_kernel_ehdr(&my_package);

	/* read ELF files' program headers */  
    my_package->boot_program_header = read_bootblock_phdr(&my_package);
    my_package->kernel_program_header = read_kernel_phdr(&my_package);

    /* Counts the number of sectors in the ELF files */
    count_bootblock_sectors(&my_package);
    count_kernel_sectors(&my_package);

	/* build image file */
    build_image(&my_package);

	/* check for --extended option */
	if(!strncmp(argv[1], "--extended", 11)) {
		extended_opt(my_package);
	}
	
	// Clean up
    printf("freeing pointers...\n");
    free(my_package->boot_elf_header);
    free(my_package->kernel_elf_header);
    free(my_package->boot_program_header);
    free(my_package->kernel_program_header);
    fclose(my_package->bootfile);
    fclose(my_package->kernelfile);
	fclose(my_package->imagefile);
  
	return 0;
}
