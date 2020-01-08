// Local Includes
#include "material.h"
#include "nshader.h"
#include "mesh.h"

// External includes
#include <nap/logger.h>
#include <GL/glew.h>
#include "rtti/rttiutilities.h"
#include "renderservice.h"
#include "nshaderutils.h"
#include "uniforminstances.h"

RTTI_BEGIN_ENUM(nap::EBlendMode)
	RTTI_ENUM_VALUE(nap::EBlendMode::NotSet,				"NotSet"),
	RTTI_ENUM_VALUE(nap::EBlendMode::Opaque,				"Opaque"),
	RTTI_ENUM_VALUE(nap::EBlendMode::AlphaBlend,			"AlphaBlend"),
	RTTI_ENUM_VALUE(nap::EBlendMode::Additive,				"Additive")
RTTI_END_ENUM

RTTI_BEGIN_ENUM(nap::EDepthMode)
	RTTI_ENUM_VALUE(nap::EDepthMode::NotSet,				"NotSet"),
	RTTI_ENUM_VALUE(nap::EDepthMode::InheritFromBlendMode,	"InheritFromBlendMode"),
	RTTI_ENUM_VALUE(nap::EDepthMode::ReadWrite,				"ReadWrite"),
	RTTI_ENUM_VALUE(nap::EDepthMode::ReadOnly,				"ReadOnly"),
	RTTI_ENUM_VALUE(nap::EDepthMode::WriteOnly,				"WriteOnly"),
	RTTI_ENUM_VALUE(nap::EDepthMode::NoReadWrite,			"NoReadWrite")
RTTI_END_ENUM

RTTI_BEGIN_STRUCT(nap::Material::VertexAttributeBinding)
	RTTI_VALUE_CONSTRUCTOR(const std::string&, const std::string&)
	RTTI_PROPERTY("MeshAttributeID",			&nap::Material::VertexAttributeBinding::mMeshAttributeID, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("ShaderAttributeID",			&nap::Material::VertexAttributeBinding::mShaderAttributeID, nap::rtti::EPropertyMetaData::Required)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::MaterialInstanceResource)
	RTTI_PROPERTY("Material",					&nap::MaterialInstanceResource::mMaterial,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Uniforms",					&nap::MaterialInstanceResource::mUniforms,	nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Samplers",					&nap::MaterialInstanceResource::mSamplers,	nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("BlendMode",					&nap::MaterialInstanceResource::mBlendMode,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("DepthMode",					&nap::MaterialInstanceResource::mDepthMode,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::MaterialInstance)
	RTTI_FUNCTION("getOrCreateUniform",			(nap::Uniform* (nap::MaterialInstance::*)(const std::string&)) &nap::MaterialInstance::getOrCreateUniform);
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::Material)
	RTTI_PROPERTY("Uniforms",					&nap::Material::mUniforms,					nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Samplers",					&nap::Material::mSamplers,					nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Shader",						&nap::Material::mShader,					nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("VertexAttributeBindings",	&nap::Material::mVertexAttributeBindings,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("BlendMode",					&nap::Material::mBlendMode,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("DepthMode",					&nap::Material::mDepthMode,					nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{


	template<class T>
	const UniformInstance* findUniformStructInstanceMember(const std::vector<T>& members, const std::string& name)
	{
		for (auto& member : members)
			if (member->getDeclaration().mName == name)
				return member.get();

		return nullptr;
	}

	template<class T>
	const Uniform* findUniformStructMember(const std::vector<T>& members, const opengl::UniformDeclaration& declaration)
	{
		for (auto& member : members)
			if (member->mName == declaration.mName)
				return member.get();

		return nullptr;
	}

	template<class T>
	const Sampler* findSamplerResource(const std::vector<T>& samplers, const opengl::SamplerDeclaration& declaration)
	{
		for (auto& sampler : samplers)
			if (sampler->mName == declaration.mName)
				return sampler.get();

		return nullptr;
	}

	template<typename INSTANCE_TYPE, typename RESOURCE_TYPE, typename DECLARATION_TYPE>
	std::unique_ptr<INSTANCE_TYPE> createUniformValueInstance(const Uniform* value, const DECLARATION_TYPE& declaration, utility::ErrorState& errorState)
	{
		std::unique_ptr<INSTANCE_TYPE> result = std::make_unique<INSTANCE_TYPE>(declaration);

		if (value != nullptr)
		{
			const RESOURCE_TYPE* typed_resource = rtti_cast<const RESOURCE_TYPE>(value);
			if (!errorState.check(typed_resource != nullptr, "Encountered type mismatch between uniform in material and uniform in shader"))
				return nullptr;
			
			result->set(*typed_resource);
		}

		return result;
	}


	template<typename T>
	std::unique_ptr<TypedUniformValueArrayInstance<T>> createUniformValueArrayInstance(const Uniform* value, const opengl::UniformValueArrayDeclaration& declaration, utility::ErrorState& errorState)
	{
		std::unique_ptr<TypedUniformValueArrayInstance<T>> result = std::make_unique<TypedUniformValueArrayInstance<T>>(declaration);

		if (value != nullptr)
		{
			const TypedUniformValueArray<T>* typed_resource = rtti_cast<const TypedUniformValueArray<T>>(value);
			if (!errorState.check(typed_resource != nullptr, "Encountered type mismatch between uniform in material and uniform in shader"))
				return nullptr;

			result->setValues(typed_resource->mValues);
		}

		return result;
	}

	bool addUniformRecursive(const opengl::UniformStructDeclaration& structDeclaration, UniformStructInstance& structInstance, const UniformStruct* structResource, const UniformCreatedCallback& uniformCreatedCallback, bool createDefaults, utility::ErrorState& errorState)
	{
		for (auto& uniform_declaration : structDeclaration.mMembers)
		{
			rtti::TypeInfo declaration_type = uniform_declaration->get_type();

			const Uniform* resource = nullptr;
			if (structResource != nullptr)
				resource = findUniformStructMember(structResource->mUniforms, *uniform_declaration);

			if (!createDefaults && resource == nullptr)
				continue;

			if (declaration_type == RTTI_OF(opengl::UniformStructArrayDeclaration))
			{
				const UniformStructArray* struct_array_resource = rtti_cast<const UniformStructArray>(resource);

				opengl::UniformStructArrayDeclaration* struct_array_declaration = rtti_cast<opengl::UniformStructArrayDeclaration>(uniform_declaration.get());
				std::unique_ptr<UniformStructArrayInstance> struct_array_instance = std::make_unique<UniformStructArrayInstance>(*struct_array_declaration);

				if (!errorState.check(struct_array_resource->mStructs.size() == struct_array_declaration->mElements.size(), "Mismatch between number of array elements in shader and json."))
					return false;

				int resource_index = 0;
				for (auto& declaration_element : struct_array_declaration->mElements)
				{
					UniformStruct* struct_resource = nullptr;
					if (struct_array_resource != nullptr && resource_index < struct_array_resource->mStructs.size())
						struct_resource = struct_array_resource->mStructs[resource_index++].get();

					std::unique_ptr<UniformStructInstance> instance_element = std::make_unique<UniformStructInstance>(*declaration_element, uniformCreatedCallback);
					if (!addUniformRecursive(*declaration_element, *instance_element, struct_resource, uniformCreatedCallback, createDefaults, errorState))
						return false;

					struct_array_instance->addElement(std::move(instance_element));
				}

				structInstance.addUniform(std::move(struct_array_instance));
			}
			else if (declaration_type == RTTI_OF(opengl::UniformValueArrayDeclaration))
			{
				opengl::UniformValueArrayDeclaration* value_array_declaration = rtti_cast<opengl::UniformValueArrayDeclaration>(uniform_declaration.get());
				std::unique_ptr<UniformValueArrayInstance> instance_value_array;

				const UniformValueArray* value_array_resource = rtti_cast<const UniformValueArray>(resource);
				if (!errorState.check(value_array_resource != nullptr, "Type mismatch between shader type and json type"))
					return false;

				if (value_array_declaration->mElementType == opengl::EGLSLType::Int)
				{
					instance_value_array = createUniformValueInstance<UniformIntArrayInstance, UniformIntArray>(resource, *value_array_declaration, errorState);
				}
				else if (value_array_declaration->mElementType == opengl::EGLSLType::Float)
				{
					instance_value_array = createUniformValueInstance<UniformFloatArrayInstance, UniformFloatArray>(resource, *value_array_declaration, errorState);
				}
				else if (value_array_declaration->mElementType == opengl::EGLSLType::Vec2)
				{
					instance_value_array = createUniformValueInstance<UniformVec2ArrayInstance, UniformVec2Array>(resource, *value_array_declaration, errorState);
				}
				else if (value_array_declaration->mElementType == opengl::EGLSLType::Vec3)
				{
					instance_value_array = createUniformValueInstance<UniformVec3ArrayInstance, UniformVec3Array>(resource, *value_array_declaration, errorState);
				}
				else if (value_array_declaration->mElementType == opengl::EGLSLType::Vec4)
				{
					instance_value_array = createUniformValueInstance<UniformVec4ArrayInstance, UniformVec4Array>(resource, *value_array_declaration, errorState);
				}
				else if (value_array_declaration->mElementType == opengl::EGLSLType::Mat4)
				{
					instance_value_array = createUniformValueInstance<UniformMat4ArrayInstance, UniformMat4Array>(resource, *value_array_declaration, errorState);
				}
				else
				{
					assert(false);
				}

				if (instance_value_array == nullptr)
					return false;

				if (!errorState.check(value_array_resource->getCount() == value_array_declaration->mNumElements, "Encountered mismatch in array elements between array in material and array in shader"))
					return false;

				structInstance.addUniform(std::move(instance_value_array));
			}
			else if (declaration_type == RTTI_OF(opengl::UniformStructDeclaration))
			{
				const UniformStruct* struct_resource = rtti_cast<const UniformStruct>(resource);

				opengl::UniformStructDeclaration* struct_declaration = rtti_cast<opengl::UniformStructDeclaration>(uniform_declaration.get());
				std::unique_ptr<UniformStructInstance> struct_instance = std::make_unique<UniformStructInstance>(*struct_declaration, uniformCreatedCallback);
				if (!addUniformRecursive(*struct_declaration, *struct_instance, struct_resource, uniformCreatedCallback, createDefaults, errorState))
					return false;

				structInstance.addUniform(std::move(struct_instance));
			}
			else
			{
				opengl::UniformValueDeclaration* value_declaration = rtti_cast<opengl::UniformValueDeclaration>(uniform_declaration.get());
				std::unique_ptr<UniformValueInstance> value_instance;

				if (value_declaration->mType == opengl::EGLSLType::Int)
				{
					value_instance = createUniformValueInstance<UniformIntInstance, UniformInt>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mType == opengl::EGLSLType::Float)
				{
					value_instance = createUniformValueInstance<UniformFloatInstance, UniformFloat>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mType == opengl::EGLSLType::Vec2)
				{
					value_instance = createUniformValueInstance<UniformVec2Instance, UniformVec2>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mType == opengl::EGLSLType::Vec3)
				{
					value_instance = createUniformValueInstance<UniformVec3Instance, UniformVec3>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mType == opengl::EGLSLType::Vec4)
				{
					value_instance = createUniformValueInstance<UniformVec4Instance, UniformVec4>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mType == opengl::EGLSLType::Mat4)
				{
					value_instance = createUniformValueInstance<UniformMat4Instance, UniformMat4>(resource, *value_declaration, errorState);
				}
				else
				{
					assert(false);
				}

				if (value_instance == nullptr)
					return false;

				structInstance.addUniform(std::move(value_instance));
			}
		}

		return true;
	}

	void buildUniformBufferObjectRecursive(const UniformStructInstance& baseInstance, const UniformStructInstance* overrideInstance, UniformBufferObject& ubo)
	{
		for (auto& base_uniform : baseInstance.getUniforms())
		{
			rtti::TypeInfo declaration_type = base_uniform->get_type();

			const UniformInstance* override_uniform = nullptr;
			if (overrideInstance != nullptr)
				override_uniform = findUniformStructInstanceMember(overrideInstance->getUniforms(), base_uniform->getDeclaration().mName);

			if (declaration_type == RTTI_OF(UniformStructArrayInstance))
			{
				const UniformStructArrayInstance* struct_array_override = rtti_cast<const UniformStructArrayInstance>(override_uniform);
				const UniformStructArrayInstance* struct_array_declaration = rtti_cast<const UniformStructArrayInstance>(base_uniform.get());

				int resource_index = 0;
				for (auto& base_element : struct_array_declaration->getElements())
				{
					UniformStructInstance* element_override = nullptr;
					if (struct_array_override != nullptr && resource_index < struct_array_override->getElements().size())
						element_override = struct_array_override->getElements()[resource_index++].get();

					buildUniformBufferObjectRecursive(*base_element, element_override, ubo);
				}
			}
			else if (declaration_type == RTTI_OF(UniformValueArrayInstance))
			{
				const UniformValueArrayInstance* base_array_uniform = rtti_cast<const UniformValueArrayInstance>(base_uniform.get());
				const UniformValueArrayInstance* override_array_uniform = rtti_cast<const UniformValueArrayInstance>(override_uniform);

				if (override_array_uniform != nullptr)
					ubo.mUniforms.push_back(override_array_uniform);
				else
					ubo.mUniforms.push_back(base_array_uniform);
			}
			else if (declaration_type == RTTI_OF(UniformStructInstance))
			{
				const UniformStructInstance* base_struct = rtti_cast<const UniformStructInstance>(base_uniform.get());
				const UniformStructInstance* override_struct = rtti_cast<const UniformStructInstance>(override_uniform);

				buildUniformBufferObjectRecursive(*base_struct, override_struct, ubo);
			}
			else
			{
				const UniformValueInstance* base_value = rtti_cast<const UniformValueInstance>(base_uniform.get());
				const UniformValueInstance* override_value = rtti_cast<const UniformValueInstance>(override_uniform);

				if (override_value != nullptr)
					ubo.mUniforms.push_back(override_value);
				else
					ubo.mUniforms.push_back(base_value);
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// MaterialInstance
	//////////////////////////////////////////////////////////////////////////

	UniformStructInstance& MaterialInstance::getOrCreateUniform(const std::string& name)
	{
		UniformStructInstance* existing = findUniform(name);
		if (existing != nullptr)
			return *existing;

		const opengl::UniformStructDeclaration* declaration = nullptr;
		const std::vector<opengl::UniformBufferObjectDeclaration>& ubo_declarations = getMaterial()->getShader()->getShader().getUBODeclarations();
		for (const opengl::UniformBufferObjectDeclaration& ubo_declaration : ubo_declarations)
		{
			if (ubo_declaration.mName == name)
			{
				declaration = &ubo_declaration;
				break;
			}
		}
		assert(declaration != nullptr);

		return createRootStruct(*declaration, std::bind(&MaterialInstance::onUniformCreated, this));
	}


	void MaterialInstance::onUniformCreated()
	{
		mUniformsDirty = true;
	}


	void MaterialInstance::rebuildUBO(UniformBufferObject& ubo, UniformStructInstance* overrideStruct)
	{
		ubo.mUniforms.clear();

		const UniformStructInstance* base_struct = rtti_cast<const UniformStructInstance>(getMaterial()->findUniform(ubo.mDeclaration->mName));
		assert(base_struct != nullptr);

		buildUniformBufferObjectRecursive(*base_struct, overrideStruct, ubo);
	}


	void MaterialInstance::updateUniforms(const DescriptorSet& descriptorSet)
	{
		VkDevice device = getMaterial()->getRenderer().getDevice();

		for (int ubo_index = 0; ubo_index != descriptorSet.mBuffers.size(); ++ubo_index)
		{
			UniformBufferObject& ubo = mUniformBufferObjects[ubo_index];
			VkDeviceMemory buffer_memory = descriptorSet.mBuffers[ubo_index].mMemory;

			void* mapped_memory;
			vkMapMemory(device, buffer_memory, 0, ubo.mDeclaration->mSize, 0, &mapped_memory);

			for (auto& uniform : ubo.mUniforms)
			{
				const UniformValueInstance* value_instance = rtti_cast<const UniformValueInstance>(uniform);
				if (value_instance == nullptr)
				{
					const UniformValueArrayInstance* value_array_instance = rtti_cast<const UniformValueArrayInstance>(uniform);
					assert(value_array_instance != nullptr);
					value_array_instance->push((uint8_t*)mapped_memory);
				}
				else
				{
					value_instance->push((uint8_t*)mapped_memory);
				}
			}

			vkUnmapMemory(device, buffer_memory);
		}
	}

	void MaterialInstance::updateSamplers(const DescriptorSet& descriptorSet)
	{
		int num_descriptors = mSamplers.size();
		std::vector<VkWriteDescriptorSet> sampler_descriptors;
		sampler_descriptors.resize(num_descriptors);

		std::vector<VkDescriptorImageInfo> sampler_images;

		for (int sampler_index = 0; sampler_index < mSamplers.size(); ++sampler_index)
		{
			SamplerInstance* sampler_instance = mSamplers[sampler_index];

			size_t sampler_image_start_index = sampler_images.size();
			if (sampler_instance->get_type() == RTTI_OF(Sampler2DInstance))
			{
				Sampler2DInstance* sampler_2d = rtti_cast<Sampler2DInstance>(sampler_instance);

				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = sampler_2d->mTexture2D->getImageView();
				imageInfo.sampler = sampler_instance->getSampler();

				sampler_images.push_back(imageInfo);
			}
			else if (sampler_instance->get_type() == RTTI_OF(Sampler2DArrayInstance))
			{
				Sampler2DArrayInstance* sampler_2d_array = rtti_cast<Sampler2DArrayInstance>(sampler_instance);

				for (auto& texture : sampler_2d_array->mTextures)
				{
					VkDescriptorImageInfo imageInfo = {};
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo.imageView = texture->getImageView();
					imageInfo.sampler = sampler_instance->getSampler();

					sampler_images.push_back(imageInfo);
				}
			}

			VkWriteDescriptorSet& write_descriptor_set = sampler_descriptors[sampler_index];
			write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_set.dstSet = descriptorSet.mSet;
			write_descriptor_set.dstBinding = sampler_instance->getDeclaration().mBinding;
			write_descriptor_set.dstArrayElement = 0;
			write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_descriptor_set.descriptorCount = sampler_images.size() - sampler_image_start_index;
			write_descriptor_set.pImageInfo = sampler_images.data() + sampler_image_start_index;
		}

		vkUpdateDescriptorSets(mDevice, sampler_descriptors.size(), sampler_descriptors.data(), 0, nullptr);
	}

	VkDescriptorSet MaterialInstance::update()
	{
		if (mUniformsDirty)
		{
			for (UniformBufferObject& ubo : mUniformBufferObjects)
				rebuildUBO(ubo, findUniform(ubo.mDeclaration->mName));

			mUniformsDirty = false;
		}

		const DescriptorSet& descriptor_set = mRenderService->acquireDescriptorSet(*this);

		updateUniforms(descriptor_set);
		updateSamplers(descriptor_set);

		return descriptor_set.mSet;
	}

	bool MaterialInstance::init(RenderService& renderService, MaterialInstanceResource& resource, utility::ErrorState& errorState)
	{
		mResource = &resource;
		mDevice = renderService.getRenderer().getDevice();
		mRenderService = &renderService;

		Material& material = *resource.mMaterial;
		const opengl::Shader& shader = material.getShader()->getShader();

		const std::vector<opengl::UniformBufferObjectDeclaration>& ubo_declarations = shader.getUBODeclarations();
		for (const opengl::UniformBufferObjectDeclaration& ubo_declaration : ubo_declarations)
		{
			const UniformStruct* struct_resource = rtti_cast<const UniformStruct>(findUniformStructMember(resource.mUniforms, ubo_declaration));

			UniformStructInstance* override_struct = nullptr;
			if (struct_resource != nullptr)
			{
				override_struct = &createRootStruct(ubo_declaration, std::bind(&MaterialInstance::onUniformCreated, this));
				if (!addUniformRecursive(ubo_declaration, *override_struct, struct_resource, std::bind(&MaterialInstance::onUniformCreated, this), false, errorState))
					return false;
			}

			UniformBufferObject ubo(ubo_declaration);
			rebuildUBO(ubo, override_struct);
			mUniformBufferObjects.emplace_back(std::move(ubo));
		}
		
		mUniformsDirty = false;
		
		const opengl::SamplerDeclarations& sampler_declarations = shader.getSamplerDeclarations();
		for (const opengl::SamplerDeclaration& declaration : sampler_declarations)
		{
			const Sampler* sampler = findSamplerResource(resource.mSamplers, declaration);
			SamplerInstance* sampler_instance = nullptr;
			if (sampler != nullptr)
			{
				bool is_array = declaration.mNumArrayElements > 1;

				std::unique_ptr<SamplerInstance> sampler_instance_override;
				if (is_array)
					sampler_instance_override = std::make_unique<Sampler2DArrayInstance>(mDevice, declaration, (Sampler2DArray*)sampler);
				else
					sampler_instance_override = std::make_unique<Sampler2DInstance>(mDevice, declaration, (Sampler2D*)sampler);

				if (!sampler_instance_override->init(errorState))
					return false;

				sampler_instance = sampler_instance_override.get();
				addSamplerInstance(std::move(sampler_instance_override));
			}
			else
			{
				sampler_instance = material.findSampler(declaration.mName);
			}

			mSamplers.push_back(sampler_instance);
		}

		return true;
	}


	Material* MaterialInstance::getMaterial() 
	{ 
		return mResource->mMaterial.get(); 
	}


	EBlendMode MaterialInstance::getBlendMode() const
	{
		if (mResource->mBlendMode != EBlendMode::NotSet)
			return mResource->mBlendMode;

		return mResource->mMaterial->getBlendMode();
	}


	void MaterialInstance::setBlendMode(EBlendMode blendMode)
	{
		mResource->mBlendMode = blendMode;
		pipelineStateChanged(*this, *mRenderService);
	}

	
	void MaterialInstance::setDepthMode(EDepthMode depthMode)
	{
		mResource->mDepthMode = depthMode;
		pipelineStateChanged(*this, *mRenderService);
	}


	EDepthMode MaterialInstance::getDepthMode() const
	{
		if (mResource->mDepthMode != EDepthMode::NotSet)
			return mResource->mDepthMode;

		return mResource->mMaterial->getDepthMode();
	}


	//////////////////////////////////////////////////////////////////////////
	// Material
	//////////////////////////////////////////////////////////////////////////

	Material::Material(RenderService& renderService) :
		mRenderer(&renderService.getRenderer())
	{
	}

	/**
	 * The Material init will initialize all uniforms that can be used with the bound shader. The shader contains the authoritative set of Uniforms that can be set;
	 * the Uniforms defined in the material must match the Uniforms declared by the shader. If the shader declares a Uniform that is not present in the Material, a 
	 * default Uniform will be used. 
	 
	 * To prevent us from having to check everywhere whether a uniform is present in the material or not, we create Uniforms for *all* uniforms
	 * declared by the shader, even if they're present in the material. Then, if the Uniform is also present in the material, we simply copy the existing uniform over the
	 * newly-constructed uniform. Furthermore, it is important to note why we always create new uniforms and do not touch the input data:
	 *
	 * - Because we create default uniforms with default values, the ownership of those newly created objects
	 *   lies within the Material. It is very inconvenient if half of the Uniforms is owned by the system and
	 *   half of the Uniforms by Material. Instead, Material owns all of them.
	 * - It is important to maintain a separation between the input data (mUniforms) and the runtime data. Json objects
	 *   should always be considered constant.
	 * - The separation makes it easy to build faster mappings for textures and values, and to provide a map interface
	 *   instead of a vector interface (which is what is supported at the moment for serialization).
	 *
	 * Thus, the Material init happens in two passes:
	 * - First, we create uniforms for all uniforms declared in the shader. This is a recursive process, due to the presence of arrays/struct uniforms
	 * - Then, we apply all uniforms that are present in the material (mUniforms) onto the newly-constructed uniforms. This is also a recursive process.
	 *
	 * Note that the first pass creates a 'tree' of uniforms (arrays can contain structs, which can contains uniforms, etc); the tree of uniforms defined in the material 
	 * must match the tree generated in the first pass.
	 */
	bool Material::init(utility::ErrorState& errorState)
	{
		if (!errorState.check(mShader != nullptr, "Shader not set in material %s", mID.c_str()))
			return false;

		const std::vector<opengl::UniformBufferObjectDeclaration>& ubo_declarations = mShader->getShader().getUBODeclarations();
		for (const opengl::UniformBufferObjectDeclaration& ubo_declaration : ubo_declarations)
		{
			const UniformStruct* struct_resource = rtti_cast<const UniformStruct>(findUniformStructMember(mUniforms, ubo_declaration));

			UniformStructInstance& root_struct = createRootStruct(ubo_declaration, UniformCreatedCallback());
			if (!addUniformRecursive(ubo_declaration, root_struct, struct_resource, UniformCreatedCallback(), true, errorState))
				return false;
		}

		const opengl::SamplerDeclarations& sampler_declarations = mShader->getShader().getSamplerDeclarations();
		for (const opengl::SamplerDeclaration& declaration : sampler_declarations)
		{
			if (!errorState.check(declaration.mType == opengl::SamplerDeclaration::EType::Type_2D, "Non-2D samplers are not supported"))
				return false;

			bool is_array = declaration.mNumArrayElements > 1;

			std::unique_ptr<SamplerInstance> sampler_instance;
			for (auto& sampler : mSamplers)
			{
				if (sampler->mName == declaration.mName)
				{
					bool target_is_array = sampler->get_type().is_derived_from<SamplerArray>();

					if (!errorState.check(is_array == target_is_array, "Sampler %s does not match array type of sampler in shader", sampler->mName.c_str()))
						return false;

					if (is_array)
						sampler_instance = std::make_unique<Sampler2DArrayInstance>(mRenderer->getDevice(), declaration, (Sampler2DArray*)sampler.get());
					else
						sampler_instance = std::make_unique<Sampler2DInstance>(mRenderer->getDevice(), declaration, (Sampler2D*)sampler.get());
				}
			}

			if (sampler_instance == nullptr)
			{
				if (is_array)
					sampler_instance = std::make_unique<Sampler2DArrayInstance>(mRenderer->getDevice(), declaration, nullptr);
				else
					sampler_instance = std::make_unique<Sampler2DInstance>(mRenderer->getDevice(), declaration, nullptr);
			}

			addSamplerInstance(std::move(sampler_instance));
		}

		std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layouts;
		for (int index = 0; index < ubo_declarations.size(); ++index)
		{
			const opengl::UniformBufferObjectDeclaration& uniform_buffer_object = ubo_declarations[index];

			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding			= uniform_buffer_object.mBinding;
			uboLayoutBinding.descriptorCount	= 1;
			uboLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.pImmutableSamplers = nullptr;
			uboLayoutBinding.stageFlags			= uniform_buffer_object.mStage;

			descriptor_set_layouts.push_back(uboLayoutBinding);
		}

		for (const opengl::SamplerDeclaration& declaration : sampler_declarations)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
			samplerLayoutBinding.binding			= declaration.mBinding;
			samplerLayoutBinding.descriptorCount	= 1;
			samplerLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags			= declaration.mStage;

			descriptor_set_layouts.push_back(samplerLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount	= (int)descriptor_set_layouts.size();
		layoutInfo.pBindings	= descriptor_set_layouts.data();

		if (!errorState.check(vkCreateDescriptorSetLayout(mRenderer->getDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) == VK_SUCCESS, "Failed to create descriptor set layout"))
			return false;

		return true;
	}


	const std::vector<Material::VertexAttributeBinding>& Material::sGetDefaultVertexAttributeBindings()
	{
		static std::vector<Material::VertexAttributeBinding> bindings;
		if (bindings.empty())
		{
			bindings.push_back({ VertexAttributeIDs::getPositionName(),		opengl::Shader::VertexAttributeIDs::getPositionVertexAttr() });
			bindings.push_back({ VertexAttributeIDs::getNormalName(),		opengl::Shader::VertexAttributeIDs::getNormalVertexAttr() });
			bindings.push_back({ VertexAttributeIDs::getTangentName(),		opengl::Shader::VertexAttributeIDs::getTangentVertexAttr() });
			bindings.push_back({ VertexAttributeIDs::getBitangentName(),	opengl::Shader::VertexAttributeIDs::getBitangentVertexAttr() });

			const int numChannels = 4;
			for (int channel = 0; channel != numChannels; ++channel)
			{
				bindings.push_back({ VertexAttributeIDs::GetColorName(channel), opengl::Shader::VertexAttributeIDs::getColorVertexAttr(channel) });
				bindings.push_back({ VertexAttributeIDs::getUVName(channel),	opengl::Shader::VertexAttributeIDs::getUVVertexAttr(channel) });
			}
		}
		return bindings;
	}

	const Material::VertexAttributeBinding* Material::findVertexAttributeBinding(const std::string& shaderAttributeID) const
	{
		// If no bindings are specified at all, use the default bindings. Note that we don't just initialize mVertexAttributeBindings to the default in init(),
		// because that would modify this object, which would cause the object diff during real-time editing to flag this object as 'modified', even though it's not.
		const std::vector<VertexAttributeBinding>& bindings = !mVertexAttributeBindings.empty() ? mVertexAttributeBindings : sGetDefaultVertexAttributeBindings();
		for (const VertexAttributeBinding& binding : bindings)
		{
			if (binding.mShaderAttributeID == shaderAttributeID)
			{
				return &binding;
			}
		}
		return nullptr;
	}
}