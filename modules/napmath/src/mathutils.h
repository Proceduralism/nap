#pragma once

// External Includes
#include <glm/glm.hpp>
#include <limits>
#include <utility/dllexport.h>
#include <algorithm>
#include <cmath>

namespace nap
{
	namespace math
	{
		/**
		* Maps @inValue to new range defined by parameters
		* @return interpolated value
		*/
		template<typename T>
		float fit(T value, T min, T max, T outMin, T outMax);

		/**
		 * Blend between @start and @end value based on percent
		 * @param start minumum value
		 * @param end maximum value
		 * @param percent the amount to blend between start and end, 0-1
		 */
		template<typename T>
		T lerp(const T& start, const T& end, float percent);

		/**
		 *	Clamps value between min and max
		 */
		template<typename T>
		T clamp(T value, T min, T max);

		/**
		 *Returns the minumum of the Left and Right values
		 */
		template<typename T>
		T min(T left, T right);

		/**
		 *	Returns the maximum of the Left and Right values
		 */
		template<typename T>
		T max(T left, T right);

		/**
		 *	Rounds down a value
		 */
		template<typename T>
		T floor(T value);

		/**
		 *	Rounds up a value
		 */
		template<typename T>
		T ceil(T value);

		/**
		 *	@return value raised to power of exponent
		 */
		template<typename T>
		T power(T value, T exp);

		/**
		 * @return epsilon of T
		 */
		template<typename T>
		T epsilon();

		/**
		 *	@return the maximum possible value of T
		 */
		template<typename T>
		T max();
		
		/**
		 *	@return the lowest possible value of T
		 */
		template<typename T>
		T min();

		//////////////////////////////////////////////////////////////////////////
		// Template definitions
		//////////////////////////////////////////////////////////////////////////

		template<typename T>
		float fit(T value, T min, T max, T outMin, T outMax)
		{
			T v = glm::clamp<T>(value, min, max);
			T m = max - min;
			m = (m == 0.0f) ? std::numeric_limits<T>::epsilon() : m;
			return (v - min) / (m) * (outMax - outMin) + outMin;
		}

		template<typename T>
		T clamp(T value, T min, T max)
		{
			return glm::clamp<T>(value, min, max);
		}

		template<typename T>
		T min(T left, T right)
		{
			return std::min<T>(left, right);
		}

		template<typename T>
		T max(T left, T right)
		{
			return std::max<T>(left, right);
		}

		template<typename T>
		T floor(T value)
		{
			return glm::floor(value);
		}

		template<typename T>
		T ceil(T value)
		{
			return glm::ceil(value);
		}

		template<typename T>
		T epsilon()
		{
			return std::numeric_limits<T>::epsilon();
		}

		template<typename T>
		T max()
		{
			return std::numeric_limits<T>::max();
		}

		template<typename T>
		T min()
		{
			return std::numeric_limits<T>::min();
		}

		//////////////////////////////////////////////////////////////////////////
		// Forward declarations of templated lerp functions
		//////////////////////////////////////////////////////////////////////////

		template<>
		NAPAPI float lerp<float>(const float& start, const float& end, float percent);

		template<>
		NAPAPI double lerp<double>(const double& start, const double& end, float percent);

		template<>
		NAPAPI glm::vec2 lerp<glm::vec2>(const glm::vec2& start, const glm::vec2& end, float percent);

		template<>
		NAPAPI glm::vec3 lerp<glm::vec3>(const glm::vec3& start, const glm::vec3& end, float percent);

		template<>
		NAPAPI glm::vec4 lerp<glm::vec4>(const glm::vec4& start, const glm::vec4& end, float percent);

		template<>
		NAPAPI float power<float>(float value, float exp);

		template<>
		NAPAPI double power<double>(double value, double exp);

		template<>
		NAPAPI int power<int>(int value, int exp);
	}
}
