#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>
#include <learnopengl/collision_utils.h>



#include <iostream>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void CreatingSphere(std::vector<float>& vertex, std::vector<unsigned int>& indices);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 5.0f, -1.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 modelPosition(0.0f, 5.0f, 0.0f);
float modelYaw = 0.0f; // หมุนโมเดลเอง (เวลาหัน)
float orbitYaw = 0.0f;   // มุมกล้องรอบโมเดล (แนวนอน)
float orbitPitch = 20.0f; // มุมกล้องขึ้น/ลง
float cameraDistance = 4.0f; // ระยะกล้องจากโมเดล

bool isWalking = false;

bool hasJump = false;
bool onGround = false;
float jumpVelocity = 0.0f;
float gravity = -9.8f;
float jumpStrength = 7.5f;   // ความแรงการกระโดด (ปรับได้)
float groundHeight = 0.7f;   // พื้นอยู่ที่ y = 0

bool punching = false;
float punchingDuration = 0;

bool animationLocked = false;     // ห้ามเปลี่ยน animation ถ้ายังเล่นไม่จบ
float currentAnimTime = 0.0f;     // เวลา animation ปัจจุบัน
float currentAnimDuration = 0.0f; // ระยะเวลา animation ปัจจุบัน

bool isJumpLoop = true;   // jump ไม่ loop
bool isWalkLoop = true;    // walk loop
bool isStandLoop = true;   // stand loop
bool isPunchLoop = true;

float jumpAnimSpeed = 0.95f;

bool jumpKeyPressed = false;
bool punchKeyPressed = false;
bool changeCamKeyPressed = false;


bool CheckMapCollision(const glm::vec3& pos, float radius, const Model& mapModel)
{
	for (const auto& mesh : mapModel.meshes)
	{
		const auto& verts = mesh.vertices;
		const auto& idx = mesh.indices;

		for (size_t i = 0; i < idx.size(); i += 3)
		{
			glm::vec3 a = verts[idx[i]].Position;
			glm::vec3 b = verts[idx[i + 1]].Position;
			glm::vec3 c = verts[idx[i + 2]].Position;

			glm::vec3 closest;
			if (TestSphereTriangle(pos, radius, a, b, c, closest))
			{
				return true;  // ชน map
			}
		}
	}

	return false; // ไม่ชน
}

float ResolveVerticalCollision(glm::vec3& pos, float radius, const Model& mapModel, float currentVelocityY)
{
	float floorY = -9999.0f;
	bool foundFloor = false;

	for (const auto& mesh : mapModel.meshes)
	{
		const auto& verts = mesh.vertices;
		const auto& idx = mesh.indices;

		for (size_t i = 0; i < idx.size(); i += 3)
		{
			glm::vec3 a = verts[idx[i]].Position;
			glm::vec3 b = verts[idx[i + 1]].Position;
			glm::vec3 c = verts[idx[i + 2]].Position;

			glm::vec3 closest;

			if (TestSphereTriangle(pos, radius, a, b, c, closest))
			{
				// ตรวจว่าชนด้านบนหรือด้านล่าง
				if (closest.y < pos.y)
				{
					// triangle อยู่ใต้ player → อาจเป็นพื้น
					if (closest.y > floorY)
					{
						floorY = closest.y;
						foundFloor = true;
					}
				}
			}
		}
	}

	if (foundFloor)
	{
		float newY = floorY + radius;
		pos.y = newY;

		return 0.0f; // reset ความเร็ว Y
	}

	return currentVelocityY;
}

static Model* gMapModel;

glm::vec3 spawnPoint(0.0f, 5.0f, 0.0f);
float initialYVelocity = 0.0f;
void Respawn()
{
	modelPosition = spawnPoint;
	jumpVelocity = 0.0f;
	onGround = true;
	hasJump = true;
}


int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader ourShader(
		"anim_model.vs",
		"anim_model.fs"
	);

	Shader skyShader(
		"sky.vs",
		"sky.fs"
	);

	// load models
	// -----------
	Model ourModel("_rooster/objects/catman/CatBoi_Walk.dae");
	Model mapModel("_rooster/objects/map/Map.obj");
	gMapModel = &mapModel;
	Animation walkAnimation(("_rooster/objects/catman/CatBoi_Walk.dae"), &ourModel);
	Animation standAnimation(("_rooster/objects/catman/CatBoi_Idle.dae"), &ourModel);
	Animation jumpAnimation(("_rooster/objects/catman/CatBoi_Jump.dae"), &ourModel);
	Animation punchAnimation(("_rooster/objects/catman/CatBoi_Punch.dae"), &ourModel);
	Animator animator(&standAnimation);
	jumpAnimation.setLoopKey(50.0f);

	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);



	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	// normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	// texcoords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindVertexArray(0);

	unsigned int planeTexture;
	glGenTextures(1, &planeTexture);
	glBindTexture(GL_TEXTURE_2D, planeTexture);

	// set the texture wrapping/filtering options (on currently bound texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char* data = stbi_load("_rooster/textures/brick001.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = GL_RGB;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	// skybox VAO
	std::vector<float> skyVert;
	std::vector<unsigned int> skyInd;
	CreatingSphere(skyVert, skyInd);
	unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glGenBuffers(1, &skyboxEBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, skyVert.size() * sizeof(float), skyVert.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, skyInd.size() * sizeof(unsigned int), skyInd.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	unsigned int skyTexture;
	glGenTextures(1, &skyTexture);
	glBindTexture(GL_TEXTURE_2D, skyTexture);

	// set the texture wrapping/filtering options (on currently bound texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	unsigned char* data2 = stbi_load("_rooster/textures/SkyCat.png", &width, &height, &nrChannels, 0);
	if (data2)
	{
		GLenum format = GL_RGB;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data2);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data2);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// ========== UPDATE JUMP PHYSICS ==========
		// Apply gravity
		jumpVelocity += gravity * deltaTime;
		modelPosition.y += jumpVelocity * deltaTime;

		jumpVelocity = ResolveVerticalCollision(modelPosition, 0.7f, mapModel, jumpVelocity);

		// ===== Respawn if player falls off map =====
		if (modelPosition.y < -50.0f)   // ตกลึกเกิน -50
		{
			Respawn();
		}


		if (jumpVelocity == 0.0f)
		{
			onGround = true;
			hasJump = true;
		}
		else
		{
			onGround = false;
		}






		// input
		// -----
		processInput(window);
		// เลือก animation ตามสถานะ
		Animation* desiredAnim = nullptr;

		if (punching) {
			desiredAnim = &punchAnimation;
			punchingDuration -= deltaTime;
			if (punchingDuration <= 0.0f) { punching = false; }
		}
		else if (!hasJump)
			desiredAnim = &jumpAnimation;
		else if (isWalking)
			desiredAnim = &walkAnimation;
		else
			desiredAnim = &standAnimation;

		// สั่งเปลี่ยน animation เฉพาะตอนที่ไม่ถูกล็อก
		if (!animationLocked && animator.GetCurrentAnimation() != desiredAnim)
		{
			animator.PlayAnimation(desiredAnim);

			animationLocked = false; // loop animation ไม่ล็อก
		}

		// เลือก animation เฉพาะตอนที่ไม่ถูกล็อก
		Animation* currentAnim;
		currentAnim = desiredAnim;

		if (!animationLocked && animator.GetCurrentAnimation() != currentAnim)
		{
			animator.PlayAnimation(currentAnim);
		}

		float animDelta = deltaTime;

		// ถ้ากำลังกระโดด → เร่ง animation
		if (!hasJump)
			animDelta *= jumpAnimSpeed;

		animator.UpdateAnimation(animDelta);

		// ====== ตรวจว่า animation non-loop เล่นครบหรือยัง ======
		if (animationLocked)
		{
			currentAnimTime += deltaTime;

			if (currentAnimTime >= currentAnimDuration)
			{
				animationLocked = false; // ปลดล็อก
			}
		}

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// don't forget to enable shader before setting uniforms
		ourShader.use();


		// คำนวณตำแหน่งกล้องตามมุม orbit
		float yawRad = glm::radians(orbitYaw);
		float pitchRad = glm::radians(orbitPitch);

		glm::vec3 cameraOffset;
		cameraOffset.x = cameraDistance * cos(pitchRad) * sin(yawRad);
		cameraOffset.y = cameraDistance * sin(pitchRad);
		cameraOffset.z = -cameraDistance * cos(pitchRad) * cos(yawRad);

		// ตำแหน่งกล้อง
		camera.Position = modelPosition + cameraOffset;

		// กล้องหันไปที่หัวโมเดลแทนพื้น
		glm::vec3 headOffset(0.0f, 0.8f, 0.0f);   // ปรับความสูงตามต้องการ
		glm::vec3 target = modelPosition + headOffset;

		camera.Front = glm::normalize(target - camera.Position);
		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
		glm::mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);
		ourShader.setVec3("sunDirection", glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f)));
		ourShader.setVec3("sunColor", glm::vec3(1.0f, 1.0f, 0.95f)); // slightly warm
		ourShader.setFloat("sunIntensity", 1.0f);   // 5 is very bright
		ourShader.setFloat("shininess", 64.0f);
		ourShader.setVec3("viewPos", camera.Position);

		auto transforms = animator.GetFinalBoneMatrices();
		for (int i = 0; i < transforms.size(); ++i)
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

		modelYaw = -orbitYaw; // ทำให้โมเดลหันหน้าหากล้อง

		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, modelPosition);
		model = glm::rotate(model, glm::radians(modelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));
		ourShader.setMat4("model", model);
		ourModel.Draw(ourShader);

		// render plane with texture
		glm::mat4 modelPlane = glm::mat4(1.0f);
		ourShader.setMat4("model", modelPlane);
		ourShader.setBool("useTexture", true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, planeTexture);
		ourShader.setInt("texture1", 0);
		//glBindVertexArray(planeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// Draw Map Model
		ourShader.use();
		glm::mat4 mapModelMatrix = glm::mat4(1.0f);
		ourShader.setMat4("model", mapModelMatrix);
		mapModel.Draw(ourShader);

		//SkyBoxRender
		glDepthFunc(GL_LEQUAL);
		skyShader.use();
		skyShader.setMat4("uProjection", projection);
		skyShader.setMat4("uView", glm::mat4(glm::mat3(view)));
		glm::mat4 skyModel = glm::mat4(1.0f);
		skyModel = glm::rotate(skyModel, glm::radians(90.0f), glm::vec3(1, 0, 0)); // rotate sky 90° around Y
		skyModel = glm::scale(skyModel, glm::vec3(2000.f));                         // scale outward
		skyShader.setMat4("uModel", skyModel);

		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skyTexture);
		skyShader.setInt("uSkyTex", 0);
		glDrawElements(GL_TRIANGLES, skyInd.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float moveSpeed = 4.0f * deltaTime;

	// ทิศทางที่โมเดลหัน (จาก modelYaw)
	glm::vec3 forward(
		sin(glm::radians(modelYaw)),
		0.0f,
		cos(glm::radians(modelYaw))
	);
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

	bool anyKeyPressed = false;

	auto TryMove = [&](glm::vec3 direction)
		{
			glm::vec3 attempt = modelPosition + direction * moveSpeed;

			float radius = 0.5f;

			// ถ้าชน → ห้ามขยับ
			if (!CheckMapCollision(attempt, radius, *gMapModel))
			{
				modelPosition = attempt;
			}
		};


	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		TryMove(forward);
		isWalking = true;
		anyKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		TryMove(-forward);
		isWalking = true;
		anyKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		TryMove(-right);
		isWalking = true;
		anyKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		TryMove(right);
		isWalking = true;
		anyKeyPressed = true;
	}


	// ถ้าไม่มีปุ่ม WASD กด
	if (!anyKeyPressed)
	{
		isWalking = false;
	}

	// ปุ่มกระโดด
	int spaceState = glfwGetKey(window, GLFW_KEY_SPACE);

	// ถ้าเพิ่งกด Space ลงครั้งแรก
	bool spacePressedNow = (spaceState == GLFW_PRESS && !jumpKeyPressed);

	// อัปเดตสถานะกดค้าง
	jumpKeyPressed = (spaceState == GLFW_PRESS);

	// กระโดดเฉพาะเงื่อนไข:
	// 1) เพิ่งกด Space
	// 2) canJump == true (ผ่าน cooldown แล้ว)
	// 3) isJumping == false (อยู่พื้นเท่านั้น) --> ไร้สาระ
	if (spacePressedNow && hasJump)
	{
		hasJump = false;
		jumpVelocity = jumpStrength;
	}

	//Punchy
	int punchState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

	// ถ้าเพิ่งกด Space ลงครั้งแรก
	bool punchPressedNow = (punchState == GLFW_PRESS && !punchKeyPressed);

	// อัปเดตสถานะกดค้าง
	punchKeyPressed = (punchState == GLFW_PRESS);

	if (punchPressedNow && punchingDuration <= 0)
	{
		punching = true;
		punchingDuration = 1;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	static float lastX = SCR_WIDTH / 2.0f;
	static float lastY = SCR_HEIGHT / 2.0f;
	static bool firstMouse = true;

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // ย้อนเพราะแกน Y ของจอคอมกลับด้าน
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	orbitYaw += xoffset;
	orbitPitch += yoffset;

	// จำกัดมุมกล้องไม่ให้หมุนเกินหัว/ล่างสุด
	if (orbitPitch > 89.0f)
		orbitPitch = 89.0f;
	if (orbitPitch < -45.0f)
		orbitPitch = -45.0f;
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

inline void CreatingSphere(std::vector<float>& vertex, std::vector<unsigned int>& indices) {
	//Stat
	float radius = 1;
	unsigned int sectorCount = 50;
	unsigned int stackCount = 25;
	vertex.clear();
	indices.clear();

	//----------------------- Vertice -----------------------------------------
	float x, y, z, xy;                              // vertex position
	float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
	float s, t;                                     // vertex texCoord

	//PosStepCal
	float sectorStep = 2 * glm::pi<float>() / sectorCount;
	float stackStep = glm::pi<float>() / stackCount;
	float sectorAngle, stackAngle;

	for (int i = 0; i <= stackCount; ++i)
	{
		stackAngle = glm::pi<float>() / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// first and last vertices have same position and normal, but different tex coords
		for (int j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;

			s = (float)j / (sectorCount / 2);
			t = (float)i / stackCount;
			vertex.push_back(x);
			vertex.push_back(y);
			vertex.push_back(z);
			vertex.push_back(nx);
			vertex.push_back(ny);
			vertex.push_back(nz);
			vertex.push_back(s);
			vertex.push_back(t);
		}
	}

	//------------------------------- Indices ---------------------------------------
	int k1, k2;
	for (int i = 0; i < stackCount; ++i)
	{
		k1 = i * (sectorCount + 1);     // beginning of current stack
		k2 = k1 + sectorCount + 1;      // beginning of next stack

		for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
	}
}