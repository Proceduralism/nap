#pragma once

// Local Includes
#include "ntexture.h"
#include "nbitmap.h"

// External Includes
#include <GL/glew.h>

namespace opengl
{
	/**
	* getGLType
	*
	* @return: the associated OpenGL system type based on the Bitmap's data type, GL_INVALID_ENUM if not found
	*/
	GLenum		getGLType(BitmapDataType type);


	/**
	 * getGLInternalFormat
	 *
	 * @return: the GL associated internal format associated with the BitmapColorType
	 * @param: compressed, if the internal format should be a compatible compressed format
	 * If no compression is available, the default uncompressed format is returned 
	 * GL_INVALID_VALUE is returned if no format is available at all
	 */
	GLint		getGLInternalFormat(BitmapColorType type, bool compressed = false);


	/**
	 * getGLFormat
	 * @return: the GL associated format associated with the bitmap's color type
	 * returns GL_INVALID_ENUM if no format is available at all
	 * The format determines the composition of each element in the texture
	 */
	GLenum		getGLFormat(BitmapColorType type);


	/**
	 * setFromBitmap
	 *
	 * Populates a Texture2D object with settings derived from the bitmap
	 * Note that this call does NOT initialize the texture object
	 * @return: If the Texture2D object is populated correctly
	 * @param bitmap: the bitmap to find matching Texture2D settings for
	 * @param texture: the texture to populate based on @bitmap
	 * @param compress: if the texture should be compressed when uploaded to the GPU  
	 */
	bool		setFromBitmap(Texture2D& texture, const BitmapBase& bitmap, bool compress = false);


	/**
	 * createFromBitmap
	 *
	 * Creates a new Texture2D object with settings derived from the bitmap
	 * Similar to setFromBitmap, only returns a new Texture2D object
	 * This call DOES initialize the texture 2D object, otherwise it can't be set
	 * @param return: new 2D object or nullptr if not successful
	 */
	Texture2D*	createFromBitmap(const BitmapBase& bitmap, bool compress = false);

	/**
	 * isCompressed
	 *
	 * checks if the texture is compressed on the GPU
	 * @param: size, the size in bytes (I believe), if not compressed this will be 0
	 * @param: type, the GPU compression type, will be invalid if texture isn't compressed
	 */
	bool isCompressed(BaseTexture& texture, GLint& size, GLint& type);
}