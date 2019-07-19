#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader_s.h"
#include <random>


#define DEFAULT_PARTICLE_NUMBER 100000
#define DEFAULT_PARTICLE_LIFESPAN 10000

const float gravity = 0.05f;

void rendersnow();

float randFloat01() {
	return 1.0 * rand() / RAND_MAX;
}

float randFloat(float from, float to) {
	return from + (to - from) * randFloat01();
}

int randInt(int from, int to) {
	return from + rand() % (to - from);
}


struct SnowParticle {
	glm::vec3 position;//位置
	glm::vec3 speed;//速度
	GLuint lifespan;//生命周期
};


class ParticleSystem {
public:
	//用默认或自定义的粒子数量和粒子生命周期进行初始化
	ParticleSystem(GLuint particleNumber = DEFAULT_PARTICLE_NUMBER,
		GLuint particleLifespan = DEFAULT_PARTICLE_LIFESPAN);
	//析构
	~ParticleSystem();
	//渲染函数
	void Render();
	//更新粒子的信息
	void Update(GLfloat deltaTime);

	Shader shader;

protected:
	//渲染每一粒存在的粒子
	void RenderParticle(const SnowParticle& p);
	//创建和销毁粒子，由Update函数调用
	void CreateParticle(SnowParticle p);
	void DestroyParticle(GLint index);

protected:
	//池分配器的分配单元，可以用来保存粒子信息或freelist信息
	union PoolAllocUnit {
		SnowParticle particle;
		struct Link {
			GLint mark;//判断是否为link的标志，设为1
			GLint nextIdx;//使用“栈”的数据结构存储freelist，所以使用单向链表即可
		}link;
		PoolAllocUnit() {}
	};
	//池分配器的地址
	PoolAllocUnit* mParticlePool;

	GLuint mParticleNumber;
	GLuint mParticleLifespan;
private:
	//表示一个在freelist中的粒子数组的索引（freelist栈的栈顶元素）
	GLint mFreeIndex;
};


ParticleSystem::ParticleSystem(GLuint particleNumber, GLuint particleLifespan)
	:mParticleNumber(particleNumber), mParticleLifespan(particleLifespan)
{
	//初始化时，根据粒子的数量进行动态内存分配
	mParticlePool = new PoolAllocUnit[mParticleNumber];
	//初始化freelist
	memset(mParticlePool, 0, sizeof(PoolAllocUnit)*mParticleNumber);
	mFreeIndex = 0;
	for (GLint i = 0; i < mParticleNumber; ++i) {
		mParticlePool[i].link.mark = 1;
		mParticlePool[i].link.nextIdx = i + 1;
	}
	mParticlePool[mParticleNumber - 1].link.nextIdx = -1;//-1标记当前freelist只剩最后这一个元素
}


ParticleSystem::~ParticleSystem()
{
	//释放动态内存
	delete[] mParticlePool;
}


void ParticleSystem::Render()
{
	//渲染每一个“存在”的粒子
	for (GLint i = 0; i < mParticleNumber; ++i) {
		if (mParticlePool[i].link.mark != 1) {
			RenderParticle(mParticlePool[i].particle);
		}
	}
}

void ParticleSystem::Update(GLfloat deltatime) {
	//create new paricles
	SnowParticle snowParticle;
	int newParticleNumber = randInt(50, 60);
	for (int i = 0; i < newParticleNumber; i++) {
		snowParticle.position = glm::vec3(randFloat(-1.0f, 1.0f), 10.0f, randFloat(-1.0f, 1.0f));
		snowParticle.speed = glm::vec3(randFloat(-1.0f, 1.0f), randFloat(-2.0f, -1.0f), randFloat(-1.0f, 1.0f));
		snowParticle.lifespan = mParticleLifespan;
		CreateParticle(snowParticle);
	}

	//update exist particles
	for (int i = 0; i < mParticleNumber; i++) {
		if (mParticlePool[i].link.mark != 1) {
			SnowParticle *p = &(mParticlePool[i].particle);
			p->position += p->speed * deltatime;
			
			p->speed.y -= gravity * deltatime;
			
			if (p->lifespan <= 0 || p->position.y < -10.0f) {
				DestroyParticle(i);
			}
			p->lifespan--;
		}
	}
}


void ParticleSystem::RenderParticle(const SnowParticle &p) {
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, p.position);
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));

	shader.use();

	glm::mat4 view = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
	glm::mat4 projection = glm::mat4(1.0f);

	projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	
	shader.setMat4("model", model);
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.setVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));

	rendersnow();

}

void ParticleSystem::CreateParticle(SnowParticle p)
{
	GLint index = mParticlePool[mFreeIndex].link.nextIdx;
	if (index == -1)return;//如果当前粒子数量已超过设定的最大粒子数量，则函数直接返回
	mParticlePool[mFreeIndex].particle = p;
	mFreeIndex = index;
}

void ParticleSystem::DestroyParticle(GLint index)
{
	if (index < 0 || index >= mParticleNumber)return;//索引不合法
	if (mParticlePool[index].link.mark == 1)return;//当前索引在freelist中
	mParticlePool[index].link.mark = 1;//当前索引添加到freelist
	mParticlePool[index].link.nextIdx = mFreeIndex;
	mFreeIndex = index;
}

unsigned int snowVAO = 0;
unsigned int snowVBO = 0;
void rendersnow()
{
	if (snowVAO == 0) {
		float snowVertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
		};
		glGenVertexArrays(1, &snowVAO);
		glGenBuffers(1, &snowVBO);

		glBindBuffer(GL_ARRAY_BUFFER, snowVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(snowVertices), snowVertices, GL_STATIC_DRAW);

		glBindVertexArray(snowVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	glBindVertexArray(snowVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}
