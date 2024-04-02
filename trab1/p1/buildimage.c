/* Author(s): <Your name(s) here>
 * Creates operating system image suitable for placement on a boot disk
*/
/* TODO: Comment on the status of your submission. Largely unimplemented */
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512       /* floppy sector size in bytes */
#define BOOTLOADER_SIG_OFFSET 0x1fe /* offset for boot loader signature */
// more defines...

// Reads in an executable file in ELF format
Elf32_Phdr * read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr)
{ 
	*execfile = fopen(filename, "rb");
    if (*execfile == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Read ELF header
    fread(*ehdr, 1, sizeof(Elf32_Ehdr), *execfile);

    // Allocate memory for program header table
    Elf32_Phdr *phdr_table = (Elf32_Phdr *)malloc((*ehdr)->e_phentsize * (*ehdr)->e_phnum);
    if (phdr_table == NULL) {
        perror("Memory allocation failed");
        fclose(*execfile);
        return NULL;
    }

    // Read program header table
    fseek(*execfile, (*ehdr)->e_phoff, SEEK_SET);
    fread(phdr_table, (*ehdr)->e_phentsize, (*ehdr)->e_phnum, *execfile);

	printf("%s read successfully!\n", filename);

    return phdr_table;
}

/* Writes the bootblock to the image file */
void write_bootblock(FILE **imagefile,FILE *bootfile,Elf32_Ehdr *boot_header, Elf32_Phdr *boot_phdr)
{
	if (*imagefile == NULL) {
        *imagefile = fopen("image", "wb");
        if (*imagefile == NULL) {
            perror("Error opening image file");
            exit(EXIT_FAILURE);
        }
    }
	
    // Allocate buffer for bootblock contents
    char *bootblock_buffer = (char *)malloc(boot_phdr->p_filesz);
    if (bootblock_buffer == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
	
    // Read bootblock contents from bootfile
    fseek(bootfile, boot_phdr->p_offset, SEEK_SET);
    fread(bootblock_buffer, 1, boot_phdr->p_filesz, bootfile);
	
    // Write bootblock contents to image file
    fwrite(bootblock_buffer, 1, boot_phdr->p_filesz, *imagefile);
	
    // Free allocated memory and close file
    free(bootblock_buffer);

	printf("Image file written successfully!\n");
}

/* Writes the kernel to the image file */
void write_kernel(FILE **imagefile,FILE *kernelfile,Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{ 
}

/* Counts the number of sectors in the kernel */
int count_kernel_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{   
    return 0;
}

/* Records the number of sectors in the kernel */
void record_kernel_sectors(FILE **imagefile,Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr, int num_sec)
{    
}


/* Prints segment information for --extended option */
void extended_opt(Elf32_Phdr *bph, int k_phnum, Elf32_Phdr *kph, int num_sec)
{

	/* print number of disk sectors used by the image */

  
	/* bootblock segment info */
 

	/* print kernel segment info */
  

	/* print kernel size in sectors */
}
// more helper functions...

/* MAIN */
int main(int argc, char **argv)
{
	FILE *kernelfile, *bootfile, *imagefile; // file pointers for bootblock, kernel, and image
    Elf32_Ehdr *boot_header = malloc(sizeof(Elf32_Ehdr)); // bootblock ELF header
    Elf32_Ehdr *kernel_header = malloc(sizeof(Elf32_Ehdr)); // kernel ELF header

    // Read bootblock program header
    Elf32_Phdr *boot_program_header = read_exec_file(&bootfile, "bootblock.o", &boot_header);
    if (boot_program_header == NULL) {
        fprintf(stderr, "Error reading bootblock ELF file\n");
        return 1;
    }

    // Read kernel program header
    Elf32_Phdr *kernel_program_header = read_exec_file(&kernelfile, "kernel.s", &kernel_header);
    if (kernel_program_header == NULL) {
        fprintf(stderr, "Error reading kernel ELF file\n");
        return 1;
    }

	/* build image file */
	write_bootblock(&imagefile, bootfile, boot_header, boot_program_header);

	/* read executable bootblock file */  

	/* write bootblock */  

	/* read executable kernel file */

	/* write kernel segments to image */

	/* tell the bootloader how many sectors to read to load the kernel */

	/* check for  --extended option */
	// if(!strncmp(argv[1], "--extended", 11)) {
	// 	/* print info */
	// }
	
	// Clean up
    free(boot_header);
    free(kernel_header);
    free(boot_program_header);
    free(kernel_program_header);
    fclose(bootfile);
    fclose(kernelfile);
	//fclose(imagefile);
  
	return 0;
} // ends main()



