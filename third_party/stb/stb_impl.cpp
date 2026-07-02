// STB single-translation-unit implementation file.
//
// IMPORTANT: STB_IMAGE_IMPLEMENTATION and STB_IMAGE_WRITE_IMPLEMENTATION must
// be defined in EXACTLY ONE translation unit across the entire project.
// This file is that unit. All other files that need stb functionality must
// include stb_image.h / stb_image_write.h WITHOUT defining those macros.
// Defining them in any other TU will cause ODR (One Definition Rule) violations
// and linker errors (duplicate symbol definitions).
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
