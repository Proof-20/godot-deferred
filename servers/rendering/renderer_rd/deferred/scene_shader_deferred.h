#pragma once

#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"

namespace RendererSceneRenderImplementation {

	class SceneShaderDeferred {
		struct ShaderData final : RendererRD::MaterialStorage::ShaderData {
			void set_code(const String &p_code) override;

			bool is_animated() const override;
			bool casts_shadows() const override;

			RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const override;
			Pair<ShaderRD *, RID> get_native_shader_and_version() const override;
		};

		struct MaterialData final : RendererRD::MaterialStorage::MaterialData {
			int render_priority = 0;
			RID next_pass;

			void set_render_priority(int p_priority) override;
			void set_next_pass(RID p_next_pass) override;

			bool update_parameters(const HashMap<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty) override;
		};

		static RendererRD::MaterialStorage::ShaderData *_create_shader_data();
		static RendererRD::MaterialStorage::MaterialData *_create_material_data(RendererRD::MaterialStorage::ShaderData *p_shader);

	public:
		SceneShaderDeferred();
		~SceneShaderDeferred();
	};
}
