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
	glm::vec3 position;//λ��
	glm::vec3 speed;//�ٶ�
	GLuint lifespan;//��������
};


class ParticleSystem {
public:
	//��Ĭ�ϻ��Զ�������������������������ڽ��г�ʼ��
	ParticleSystem(GLuint particleNumber = DEFAULT_PARTICLE_NUMBER,
		GLuint particleLifespan = DEFAULT_PARTICLE_LIFESPAN);
	//����
	~ParticleSystem();
	//��Ⱦ����
	void Render();
	//�������ӵ���Ϣ
	void Update(GLfloat deltaTime);

	Shader shader;

protected:
	//��Ⱦÿһ�����ڵ�����
	void RenderParticle(const SnowParticle& p);
	//�������������ӣ���Update��������
	void CreateParticle(SnowParticle p);
	void DestroyParticle(GLint index);

protected:
	//�ط������ķ��䵥Ԫ��������������������Ϣ��freelist��Ϣ
	union PoolAllocUnit {
		SnowParticle particle;
		struct Link {
			GLint mark;//�ж��Ƿ�Ϊlink�ı�־����Ϊ1
			GLint nextIdx;//ʹ�á�ջ�������ݽṹ�洢freelist������ʹ�õ���������
		}link;
		PoolAllocUnit() {}
	};
	//�ط������ĵ�ַ
	PoolAllocUnit* mParticlePool;

	GLuint mParticleNumber;
	GLuint mParticleLifespan;
private:
	//��ʾһ����freelist�е����������������freelistջ��ջ��Ԫ�أ�
	GLint mFreeIndex;
};


ParticleSystem::ParticleSystem(GLuint particleNumber, GLuint particleLifespan)
	:mParticleNumber(particleNumber), mParticleLifespan(particleLifespan)
{
	//��ʼ��ʱ���������ӵ��������ж�̬�ڴ����
	mParticlePool = new PoolAllocUnit[mParticleNumber];
	//��ʼ��freelist
	memset(mParticlePool, 0, sizeof(PoolAllocUnit)*mParticleNumber);
	mFreeIndex = 0;
	for (GLint i = 0; i < mParticleNumber; ++i) {
		mParticlePool[i].link.mark = 1;
		mParticlePool[i].link.nextIdx = i + 1;
	}
	mParticlePool[mParticleNumber - 1].link.nextIdx = -1;//-1��ǵ�ǰfreelistֻʣ�����һ��Ԫ��
}


ParticleSystem::~ParticleSystem()
{
	//�ͷŶ�̬�ڴ�
	delete[] mParticlePool;
}


void ParticleSystem::Render()
{
	//��Ⱦÿһ�������ڡ�������
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
	if (index == -1)return;//�����ǰ���������ѳ����趨�������������������ֱ�ӷ���
	mParticlePool[mFreeIndex].particle = p;
	mFreeIndex = index;
}

void ParticleSystem::DestroyParticle(GLint index)
{
	if (index < 0 || index >= mParticleNumber)return;//�������Ϸ�
	if (mParticlePool[index].link.mark == 1)return;//��ǰ������freelist��
	mParticlePool[index].link.mark = 1;//��ǰ������ӵ�freelist
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
