/**
 * @file kernel/module_image.c
 * @ingroup modimg
 *
 * @brief linker references to the embedded modules.image
 *
 * Declares the symbols produced by the linker when the modules image is
 * embedded into the kernel binary, giving the location and size of the
 * image in memory.
 */

extern unsigned char _binary_modules_image_start;
extern unsigned char _binary_modules_image_end;
extern unsigned char _binary_modules_image_size;

