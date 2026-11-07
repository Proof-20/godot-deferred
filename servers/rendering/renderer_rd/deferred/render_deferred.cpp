#include "render_deferred.h"

#include "core/error/error_macros.h"
#include "core/os/memory.h"
#include "servers/rendering/renderer_rd/framebuffer_cache_rd.h"
#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering/storage/camera_attributes_storage.h"

namespace RendererSceneRenderImplementation {
void RenderDeferred::RenderBufferDataDeferred::configure(RenderSceneBuffersRD *p_render_buffers) {
	free_data();

	ERR_FAIL_NULL(p_render_buffers);
	ERR_FAIL_COND_MSG(p_render_buffers->get_view_count() != 1, "Deferred+: multiview rendering is not supported yet.");
	ERR_FAIL_COND_MSG(p_render_buffers->get_msaa_3d() != RSE::VIEWPORT_MSAA_DISABLED, "Deferred+: MSAA is not supported yet.");
	ERR_FAIL_COND_MSG(p_render_buffers->get_use_taa(), "Deferred+: TAA is not supported yet.");
	ERR_FAIL_COND_MSG(p_render_buffers->get_scaling_3d_mode() != RSE::VIEWPORT_SCALING_3D_MODE_OFF, "Deferred+: render scaling is not supported by the initialization backend.");
	ERR_FAIL_COND_MSG(p_render_buffers->get_render_target().is_null(), "Deferred+: rendering without a viewport render target is not supported yet.");

	const RID color = p_render_buffers->get_internal_texture();
	const RID depth = p_render_buffers->get_depth_texture();

	ERR_FAIL_COND(color.is_null());
	ERR_FAIL_COND(depth.is_null());

	clear_framebuffer = FramebufferCacheRD::get_singleton()->get_cache(color, depth);

	ERR_FAIL_COND_MSG(clear_framebuffer.is_null(), "Deferred+: failed to create the scene clear framebuffer.");
}

void RenderDeferred::RenderBufferDataDeferred::free_data() {
	// FramebufferCacheRD owns the framebuffer. Its texture dependencies are
	// released by RenderSceneBuffersRD, so only discard our cached reference.
	clear_framebuffer = RID();
}

RenderDeferred::GeometryInstanceDeferred::GeometryInstanceDeferred(RID p_base, RSE::InstanceType p_type) {
	data = &data_storage;
	data_storage.base = p_base;
	data_storage.base_type = p_type;
}

void RenderDeferred::GeometryInstanceDeferred::_mark_dirty() {
	data_storage.dirty_dependencies = true;
}

void RenderDeferred::setup_render_buffer_data(Ref<RenderSceneBuffersRD> p_render_buffers) {
	Ref<RenderBufferDataDeferred> data;
	data.instantiate();

	p_render_buffers->set_custom_data(SNAME("deferred"), data);
}

RenderGeometryInstance *RenderDeferred::geometry_instance_create(RID p_base) {
	const RSE::InstanceType type = RSG::utilities->get_base_type(p_base);

	ERR_FAIL_COND_V_MSG(type
		!= RSE::INSTANCE_MESH && type
		!= RSE::INSTANCE_MULTIMESH && type
		!= RSE::INSTANCE_PARTICLES, nullptr, "Deferred+: unsupported geometry instance base type.");

	GeometryInstanceDeferred *instance = memnew(GeometryInstanceDeferred(p_base, type));

	geometry_instances.insert(instance);
	return instance;
}

void RenderDeferred::geometry_instance_free(RenderGeometryInstance *p_instance) {
	ERR_FAIL_NULL(p_instance);
	ERR_FAIL_COND_MSG(!geometry_instances.erase(p_instance), "Deferred+: geometry instance does not belong to this renderer.");

	memdelete(p_instance);
}

void RenderDeferred::_render_scene(RenderDataRD *p_render_data, const Color &p_default_color) {
	ERR_FAIL_NULL(p_render_data);
	ERR_FAIL_COND(p_render_data->render_buffers.is_null());
	ERR_FAIL_COND_MSG(p_render_data->reflection_probe.is_valid(), "Deferred+: reflection-probe rendering is not supported yet.");

	const Ref<RenderSceneBuffersRD> render_buffers = p_render_data->render_buffers;
	const Ref<RenderBufferDataDeferred> data = render_buffers->get_custom_data(SNAME("deferred"));

	ERR_FAIL_COND(data.is_null());
	ERR_FAIL_COND_MSG(data->get_clear_framebuffer().is_null(), "Deferred+: render buffers are not configured for a supported viewport.");

	Color clear_color = p_default_color;
	float background_energy = 1.0f;

	if (is_environment(p_render_data->environment)) {
		const RSE::EnvironmentBG background =
			environment_get_background(p_render_data->environment);

		switch (background) {
			case RSE::ENV_BG_CLEAR_COLOR:
				break;

			case RSE::ENV_BG_COLOR:
				clear_color = environment_get_bg_color(p_render_data->environment);
				break;

			default:
				ERR_PRINT_ONCE("Deferred+: this environment background mode is not supported yet. Only clear-color and solid-color backgrounds are available.");
				break;
		}

		background_energy = environment_get_bg_energy_multiplier(p_render_data->environment) * environment_get_bg_intensity(p_render_data->environment);

		if (p_render_data->camera_attributes.is_valid())
			background_energy *= RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(p_render_data->camera_attributes);
	}

	clear_color = clear_color.srgb_to_linear();

	const float color_scale =
		background_energy * render_buffers->get_luminance_multiplier();

	clear_color.r *= color_scale;
	clear_color.g *= color_scale;
	clear_color.b *= color_scale;
	clear_color.a = 1.0f;

	if (p_render_data->transparent_bg)
		clear_color = Color(0, 0, 0, 0);

	Vector<Color> clear_colors;
	clear_colors.push_back(clear_color);

	RD *rd = RD::get_singleton();

	rd->draw_list_begin(data->get_clear_framebuffer(), RD::DRAW_CLEAR_COLOR_0 | RD::DRAW_CLEAR_DEPTH, clear_colors, 0.0f);
	rd->draw_list_end();

	if (p_render_data->instances && p_render_data->instances->size() != 0)
		ERR_PRINT_ONCE("Deferred+: scene geometry rendering is not implemented yet. The initialization backend only clears the scene color and depth.");

	if ((p_render_data->lights && p_render_data->lights->size() != 0) || p_render_data->directional_light_count != 0 || p_render_data->render_shadow_count != 0)
		ERR_PRINT_ONCE("Deferred+: scene lighting and shadow rendering are not supported yet.");

	_render_buffers_post_process_and_tonemap(p_render_data);
}

void RenderDeferred::_render_material(const Transform3D &, const Projection &, bool, const PagedArray<RenderGeometryInstance *> &, RID, const Rect2i &, float) {
	ERR_PRINT_ONCE("Deferred+: material baking is not supported yet.");
}

void RenderDeferred::_render_uv2(const PagedArray<RenderGeometryInstance *> &, RID, const Rect2i &) {
	ERR_PRINT_ONCE("Deferred+: UV2 baking is not supported yet.");
}

void RenderDeferred::_render_sdfgi(Ref<RenderSceneBuffersRD>, const Vector3i &, const Vector3i &, const AABB &, const PagedArray<RenderGeometryInstance *> &, const RID &, const RID &, const RID &, const RID &, float) {
	ERR_PRINT_ONCE("Deferred+: SDFGI rendering is not supported yet.");
}

void RenderDeferred::_render_particle_collider_heightfield(RID, const Transform3D &, const Projection &, const PagedArray<RenderGeometryInstance *> &) {
	ERR_PRINT_ONCE("Deferred+: particle-collider heightfield rendering is not supported yet.");
}

RenderDeferred::~RenderDeferred() {
	// Also release instances left by partial initialization or teardown.
	for (RenderGeometryInstance *instance : geometry_instances) {
		memdelete(instance);
	}

	geometry_instances.clear();
}

}
