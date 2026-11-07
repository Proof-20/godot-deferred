#pragma once

#include "scene_shader_deferred.h"

#include "core/templates/hash_set.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"

namespace RendererSceneRenderImplementation {
class RenderDeferred final : public RendererSceneRenderRD {
	class RenderBufferDataDeferred final : public RenderBufferCustomDataRD {

		GDCLASS(RenderBufferDataDeferred, RenderBufferCustomDataRD)
		RID clear_framebuffer;

	public:
		void configure(RenderSceneBuffersRD *p_render_buffers) override;
		void free_data() override;

		RID get_clear_framebuffer() const {
			return clear_framebuffer;
		}
	};

	struct GeometryInstanceDeferred final : RenderGeometryInstanceBase {
		Data data_storage;

		GeometryInstanceDeferred(RID p_base, RSE::InstanceType p_type);

		void _mark_dirty() override;

		void set_use_lightmap(RID p_lightmap, const Rect2 &, int) override {
			ERR_FAIL_COND_MSG(p_lightmap.is_valid(), "Deferred+: lightmap shading is not supported yet.");
		}

		void set_lightmap_capture(const Color *p_sh9) override {
			ERR_FAIL_COND_MSG(p_sh9 != nullptr, "Deferred+: lightmap captures are not supported yet.");
		}

		void clear_light_instances() override {
			// Pairing is rejected below, so the paired-light set is always empty.
		}

		void pair_light_instance(const RID p_light_instance, RenderingServerEnums::LightType light_type, uint32_t placement_idx) override {
			ERR_FAIL_COND_MSG(p_light_instance.is_valid(), "Deferred+: per-instance light pairing is not supported yet.");
		}

		void pair_reflection_probe_instances(const RID *, const uint32_t p_count) override {
			ERR_FAIL_COND_MSG(p_count != 0, "Deferred+: reflection-probe pairing is not supported yet.");
		}

		void pair_decal_instances(const RID *, const uint32_t p_count) override {
			ERR_FAIL_COND_MSG(p_count != 0, "Deferred+: decal pairing is not supported yet.");
		}

		void pair_voxel_gi_instances(const RID *, const uint32_t p_count) override {
			ERR_FAIL_COND_MSG(p_count != 0, "Deferred+: VoxelGI pairing is not supported yet.");
		}

		void set_softshadow_projector_pairing(bool p_softshadow, bool p_projector) override {
			ERR_FAIL_COND_MSG(p_softshadow || p_projector, "Deferred+: soft-shadow/projector pairing is not supported yet.");
		}
	};

	SceneShaderDeferred scene_shader;
	HashSet<RenderGeometryInstance *> geometry_instances;

protected:
	void setup_render_buffer_data(Ref<RenderSceneBuffersRD> p_render_buffers) override;
	void _render_scene(RenderDataRD *p_render_data, const Color &p_default_color) override;

	void _render_material(const Transform3D &, const Projection &, bool, const PagedArray<RenderGeometryInstance *> &, RID, const Rect2i &, float) override;
	void _render_uv2(const PagedArray<RenderGeometryInstance *> &, RID, const Rect2i &) override;

	void _render_sdfgi(Ref<RenderSceneBuffersRD>, const Vector3i &, const Vector3i &, const AABB &, const PagedArray<RenderGeometryInstance *> &, const RID &, const RID &, const RID &, const RID &, float) override;
	void _render_particle_collider_heightfield(RID, const Transform3D &, const Projection &, const PagedArray<RenderGeometryInstance *> &) override;

	RID _render_buffers_get_normal_texture(Ref<RenderSceneBuffersRD>) override {
		return {};
	}

	RID _render_buffers_get_velocity_texture(Ref<RenderSceneBuffersRD>) override {
		return {};
	}

	RD::DataFormat _render_buffers_get_preferred_color_format() override {
		return RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
	}

public:
	RenderGeometryInstance *geometry_instance_create(RID p_base) override;
	void geometry_instance_free(RenderGeometryInstance *p_instance) override;

	uint32_t geometry_instance_get_pair_mask() override {
		return 0;
	}

	uint32_t get_max_lights_total() override {
		return 0;
	}

	uint32_t get_max_lights_per_mesh() override {
		return 0;
	}

	void mesh_generate_pipelines(RID, bool) override {
		ERR_PRINT_ONCE("Deferred+: geometry pipelines are not supported yet.");
	}

	uint32_t get_pipeline_compilations(RSE::PipelineSource) override {
		return 0;
	}

	void base_uniforms_changed() override {
		// No scene uniform sets exist in the initialization backend.
	}

	void sdfgi_update(const Ref<RenderSceneBuffers> &, RID, const Vector3 &) override {
		ERR_PRINT_ONCE("Deferred+: SDFGI is not supported yet.");
	}

	int sdfgi_get_pending_region_count(const Ref<RenderSceneBuffers> &) const override {
		return 0;
	}

	AABB sdfgi_get_pending_region_bounds(const Ref<RenderSceneBuffers> &, int) const override {
		ERR_FAIL_V_MSG(AABB(), "Deferred+: no pending SDFGI regions exist.");
	}

	uint32_t sdfgi_get_pending_region_cascade(const Ref<RenderSceneBuffers> &, int) const override {
		ERR_FAIL_V_MSG(0, "Deferred+: no pending SDFGI regions exist.");
	}

	RID reflection_probe_create_framebuffer(RID, RID) override {
		ERR_FAIL_V_MSG(RID(), "Deferred+: reflection-probe capture is not supported yet.");
	}

	void environment_set_ssr_half_size(bool) override {
		WARN_PRINT_ONCE("Deferred+: SSR settings have no effect; SSR is not supported yet.");
	}

	void environment_set_ssr_roughness_quality(RSE::EnvironmentSSRRoughnessQuality) override {
		WARN_PRINT_ONCE("Deferred+: SSR settings have no effect; SSR is not supported yet.");
	}

	void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality,bool, float, int, float, float) override {
		WARN_PRINT_ONCE("Deferred+: SSAO settings have no effect; SSAO is not supported yet.");
	}

	void environment_set_ssil_quality(RSE::EnvironmentSSILQuality, bool, float, int, float, float) override {
		WARN_PRINT_ONCE("Deferred+: SSIL settings have no effect; SSIL is not supported yet.");
	}

	void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality) override {
		if (p_quality != RSE::SUB_SURFACE_SCATTERING_QUALITY_DISABLED) {
			WARN_PRINT_ONCE("Deferred+: subsurface scattering is not supported yet.");
		}
	}

	void sub_surface_scattering_set_scale(float, float) override {
		WARN_PRINT_ONCE("Deferred+: subsurface-scattering scale has no effect;" "subsurface scattering is not supported yet.");
	}

	bool is_vrs_supported() const override {
		return false;
	}

	bool is_dynamic_gi_supported() const override {
		return false;
	}

	bool is_volumetric_supported() const override {
		return false;
	}

	void enable_features(BitField<FeatureBits> p_feature_bits) override {
		ERR_FAIL_COND_MSG(!p_feature_bits.is_empty(), "Deferred+: scene shader baking is not supported yet.");
	}

	String get_name() const override {
		return "deferred";
	}

	RenderDeferred() = default;
	~RenderDeferred() override;
};
}
