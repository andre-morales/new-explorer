#include "Renderer.h"
#include "Client/Client.h"
#include "Client/ClientImpl.h"
#include "Client/Session.h"
#include "Glass/GUI.h"
#include "Glow/Window.h"
#include "Glow/GLContext.h"
#include "Glow/Texture.h"
#include "Glow/Inputs.h"
#include "Glow/Shader.h"
#include "GL/glew.h"

#include "Geometry.h"
#include "Client/Chunk.h"
#include "Physics/Collisions.h"
#include "Client/World.h"
#include "Client/Player.h"
#include "Loggy.h"

#define RADS (3.14159265359/180.0)

using namespace Glow;

static Loggy::Logger print {"Render"};

Shader* shader;

Renderer::Renderer() {}

void Renderer::_recompileShader() {
	shader->readShaderFile("res/shaders/world.sh");
	shader->compile();
}

void Renderer::init() {
	Client& client = Client::get();
	Glow::Window& window = *client.window;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glAlphaFunc(GL_GEQUAL, 0.5);

	{
		float lmambient[] = { 1, 1, 1, 1 };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmambient);

		float position[] = { 0.57, 0.57, 0.57, 0 };
		glLightfv(GL_LIGHT0, GL_POSITION, position);

		float ambient[] = { 1, 1, 1, 1 };
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

		float diffuse[] = { 1, 1, 1, 1 };
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

	}

	shader = new Shader();
	shader->readShaderFile("res/shaders/world.sh");
	shader->compile();
}

void Renderer::resize(int width, int height) {
	glViewport(0, 0, width, height);
}

void Renderer::render() {
	Client& client = Client::get();
	Glow::Window& window = *client.window;
	Glow::GLContext& gl = *window.getContext();
	Session* sessionPtr = client.getSession();

	glClearColor(1.0f, 0.4f, 0.7f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (sessionPtr) {
		renderSession();
	}

	client.gui->paint();

	auto error = glGetError();
	if (error != 0) {
		print("GL Error: ", error);
	}
}

void dbgRenderCubeAt(GLContext& gl, Vector3f position, float scale = 1, bool wireframe = true) {
	glPushMatrix();
	gl.disableTexture2d();

	glTranslatef(position.x, position.y, position.z);
	glScalef(scale, scale, scale);

	glLineWidth(3);
	
	if (wireframe) {
		glVertexPointer(3, GL_FLOAT, 0, Geometry::Cube::verts_lines);
		glDrawArrays(GL_LINES, 0, 24);
	} else {
		glVertexPointer(3, GL_FLOAT, 0, Geometry::Cube::verts_tris);
		glDrawArrays(GL_QUADS, 0, 24);
	}
	glPopMatrix();

	gl.enableTexture2d();
}

void doTest() {
	
}

void Renderer::renderSession() {
	Client& client = Client::get();
	Glow::Window& window = *client.window;
	Glow::GLContext& gl = *window.getContext();
	Session& session = *client.getSession();
	Player& player = session.getPlayer();
	std::vector<Chunk*>& chunks = session.getWorld().chunks;

	gl.enableDepthTesting();
	gl.enableBlending();
	gl.setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl.enableFaceCulling();
	glMatrixMode(GL_PROJECTION);

	camera.aspect = client.getWindow()->getAspect();
	camera.makeTransformMatrices();
	glLoadIdentity();
	glLoadMatrixf(camera.projectionMat.mat);

	// Render sky
	{
		gl.getTexture("sky")->bind();
		glDepthMask(GL_FALSE);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(camera.rotatedViewMat);
		
		gl.enableVertsArray();
		gl.enableUVsArray();
		glVertexPointer(3, GL_FLOAT, 0, Geometry::Skybox::verts);
		glTexCoordPointer(2, GL_FLOAT, 0, Geometry::Skybox::uvs);
		glDrawArrays(GL_QUADS, 0, 24);

		glDepthMask(GL_TRUE);
	}

	glLoadMatrixf(camera.modelViewMat.mat);

	{
		//float pos[] = { 1, 1, 0.2, 0 };
		//glLightfv(GL_LIGHT0, GL_POSITION, pos);
	}

	glColor4f(1, 1, 1, 1);
	gl.getTexture("grass")->bind();

	for (auto& chunk : chunks) {
		glPushMatrix();
		glTranslatef(chunk->cx * 16, 0, chunk->cz * 16);
		auto& verts = chunk->vertices;
		auto& uvs = chunk->uvs;
		auto& normals = chunk->normals;
		auto& colors = chunk->colors;
		if (verts.size() > 0) {
			gl.enableNormalsArray();
			gl.enableVertsArray();
			gl.enableUVsArray();
			gl.enableColorsArray();
			gl.enableLighting();
			glUseProgram(shader->glId);
			glEnable(GL_LIGHT0);

			glVertexPointer(3, GL_FLOAT, 0, &verts[0]);
			glTexCoordPointer(2, GL_FLOAT, 0, &uvs[0]);
			glNormalPointer(GL_FLOAT, 0, &normals[0]);
			glColorPointer(3, GL_FLOAT, 0, &colors[0]);
			glDrawArrays(GL_TRIANGLES, 0, verts.size());

			gl.disableNormalsArray();
			gl.disableUVsArray();
			gl.disableVertsArray();
			gl.disableColorsArray();
			gl.disableLighting();
			glUseProgram(0);
		}
		
		glPopMatrix();
	}

	{
		gl.enableVertsArray();
		
		if (camera.isLookingAtTerrain) {
			auto direction = camera.getLookingDirection();
			auto& lookTriangle = camera.lookingTerrainTriangle;

			glColor3f(0.5, 1, 0);

			Vector3f tv[]{
				lookTriangle[0] - direction * 0.01,
				lookTriangle[1] - direction * 0.01,
				lookTriangle[2] - direction * 0.01
			};

			glLineWidth(3);
			glVertexPointer(3, GL_FLOAT, 0, tv);
			glDrawArrays(GL_LINE_LOOP, 0, 3);

			auto selectedBlock_ = camera.getLookingBlockPos();
			if (selectedBlock_) {
				Vector3f selectedBlock = *selectedBlock_;
				selectedBlock += direction * -0.1;
				glColor3f(1.0, 0.5, 0.3);
				dbgRenderCubeAt(gl, selectedBlock, 1, true);
			}

			auto placeBlock_ = camera.getLookingEmptyBlockPos();
			if (placeBlock_) {
				Vector3f placeBlock = *placeBlock_;
				placeBlock += direction * -0.1;
				glColor3f(0.2, 1.0, 0.6);
				dbgRenderCubeAt(gl, placeBlock, 1, true);
			}

			glColor3f(0.3, 0.3, 1);
			dbgRenderCubeAt(gl, camera.lookingTerrainPos, 0.05, false);
			
		}

		{
			gl.disableTexture2d();
			auto direction = camera.getLookingDirection();
			AABB box;
			box.center = Vector3f{ player.position.x, player.position.y + 0.8f, player.position.z };
			box.extents = Vector3f{ .3, .8, .3 };
		
			glColor4f(1, 0, 0, 1);
			glLineWidth(3);

			auto boxv = box.vertices();

			Vector3f boxvv[]{
				boxv[0], boxv[1],
				boxv[0], boxv[2],
				boxv[0], boxv[4],
				boxv[1], boxv[3],
				boxv[1], boxv[5],
				boxv[2], boxv[3],
				boxv[2], boxv[6],
				boxv[3], boxv[7],
				boxv[4], boxv[5],
				boxv[4], boxv[6],
				boxv[5], boxv[7],
				boxv[6], boxv[7],
			};

			glVertexPointer(3, GL_FLOAT, 0, boxvv);
			glDrawArrays(GL_LINES, 0, 24);


			auto& verts = chunks[0]->vertices;
			for (int i = 0; i < verts.size(); i += 3) {
				Vector3f triangle[]{
					verts[i + 0],
					verts[i + 1],
					verts[i + 2]
				};

				if (Collisions::IsIntersecting(box, triangle)) {
					Vector3f ff[]{
						triangle[0] - direction * 0.01,
						triangle[1] - direction * 0.01,
						triangle[2] - direction * 0.01
					};

					glLineWidth(3);
					glVertexPointer(3, GL_FLOAT, 0, ff);
					glDrawArrays(GL_LINE_LOOP, 0, 3);
					//break;
				}

			}

		}
		gl.enableTexture2d();
		gl.enableDepthTesting();
		gl.disableVertsArray();
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	gl.disableDepthTesting();
	gl.disableFaceCulling();
}