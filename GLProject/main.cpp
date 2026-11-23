#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

#include <GLFW/glfw3.h>
#include "glm_mini.hpp"
#include "PolygonWithTexture.hpp"

#if defined(__has_include)
#  if __has_include("stb_image.h")
#    define STB_IMAGE_IMPLEMENTATION
#    include "stb_image.h"
#    define HAS_STB 1
#  else
#    define HAS_STB 0
#  endif
#else
#  define HAS_STB 0
#endif

using namespace glm;

int SCR_WIDTH = 1024;
int SCR_HEIGHT = 768;

// 1 = Top-down camera | 2 = Player perspective camera
int cameraMode = 2;
vec3 cameraPos = vec3(0.0f, 2.5f, 5.2f);
vec3 cameraTarget = vec3(0.0f, 0.05f, 0.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

vec3 targetCamPos = vec3(0.0f, 2.5f, 5.2f);
vec3 targetCamTarget = vec3(0.0f, 0.05f, 0.0f);

struct Card {
    int id;
    bool inPlay = false;
    vec3 currentPos;
    vec3 targetPos;
    float standFactor = 1.0f;
    float targetStandFactor = 1.0f;
    GLuint textureID = 0;
};

struct OBJVertex {
    vec3 pos;
    float u = 0.0f;
    float v = 0.0f;
};

struct OBJFace {
    std::string material;
    std::vector<OBJVertex> verts;
};

struct LoadedOBJModel {
    std::vector<OBJFace> faces;
    std::map<std::string, GLuint> materialTextures;
    bool loaded = false;
};

std::vector<Card> deckCards;
int playedCount = 0;

bool key1WasPressed = false;
bool key2WasPressed = false;
bool keyGWasPressed = false;

std::vector<vec3> makeQuad(float w, float d) {
    float hw = w * 0.5f;
    float hd = d * 0.5f;
    return {
        vec3(-hw, 0.0f,  hd),
        vec3( hw, 0.0f,  hd),
        vec3( hw, 0.0f, -hd),
        vec3(-hw, 0.0f, -hd)
    };
}

std::vector<vec3> makeVerticalQuad(float w, float h) {
    float hw = w * 0.5f;
    return {
        vec3(-hw, 0.0f, 0.0f),
        vec3( hw, 0.0f, 0.0f),
        vec3( hw, h,    0.0f),
        vec3(-hw, h,    0.0f)
    };
}

std::vector<vec3> makeTornPosterQuad(float w, float h) {
    float hw = w * 0.5f;
    return {
        vec3(-hw,         0.0f,       0.0f),
        vec3( hw,         0.0f,       0.0f),
        vec3( hw,         h * 0.52f,  0.0f),
        vec3( hw * 0.78f, h * 0.64f,  0.0f),
        vec3( hw * 0.62f, h * 0.86f,  0.0f),
        vec3( hw * 0.25f, h * 0.94f,  0.0f),
        vec3(-hw * 0.12f, h,          0.0f),
        vec3(-hw * 0.55f, h * 0.92f,  0.0f),
        vec3(-hw,         h * 0.96f,  0.0f)
    };
}

struct Polygon {
    std::vector<vec3> vertices;
    vec3 color;
    mat4 modelMatrix = mat4(1.0f);

    Polygon(const std::vector<vec3>& verts, const vec3& col)
        : vertices(verts), color(col) {}

    void transformation(const mat4& mat) {
        modelMatrix = mat;
    }

    void draw() const {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(value_ptr(modelMatrix));

        glDisable(GL_TEXTURE_2D);
        glColor3f(color.x, color.y, color.z);
        glBegin(GL_TRIANGLE_FAN);
        for (const auto& v : vertices) {
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();

        glPopMatrix();
    }
};

std::string resolvePath(const std::string& relPath) {
    size_t slash = relPath.find_last_of("/\\");
    std::string fileName = (slash == std::string::npos) ? relPath : relPath.substr(slash + 1);

    std::vector<std::string> paths = {
        relPath,
        "GLProject/" + relPath,
        "../GLProject/" + relPath,
        "C:/CardGame/GLProject/" + relPath,
        "D:/pendulum-obj/" + fileName
    };

    for (const auto& p : paths) {
        std::ifstream f(p);
        if (f.is_open()) return p;
    }
    return "";
}

GLuint loadTexture(const std::string& relPath, int fallbackId = 0) {
    int width = 0, height = 0, nrChannels = 0;
    unsigned char* data = nullptr;

#if HAS_STB
    stbi_set_flip_vertically_on_load(true);
    std::string path = resolvePath(relPath);
    if (!path.empty()) {
        data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    }
#endif

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (data && width > 0 && height > 0) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
#if HAS_STB
        stbi_image_free(data);
#endif
    } else {
        unsigned char r = 160, g = 130, b = 90;
        if (fallbackId == 1)      { r = 125; g = 65;  b = 145; }
        else if (fallbackId == 2) { r = 165; g = 45;  b = 45;  }
        else if (fallbackId == 3) { r = 45;  g = 85;  b = 165; }
        else if (fallbackId == 4) { r = 65;  g = 145; b = 60;  }
        else if (fallbackId == 5) { r = 185; g = 55;  b = 35;  }
        unsigned char pixel[3] = { r, g, b };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

std::map<std::string, GLuint> loadMTLTextures(const std::string& mtlRef) {
    std::map<std::string, GLuint> texMap;
    std::string mtlPath = resolvePath("assets/models/" + mtlRef);
    if (mtlPath.empty()) mtlPath = resolvePath(mtlRef);

    std::ifstream file(mtlPath);
    if (!file.is_open()) return texMap;

    std::string line, currentMat;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        if (key == "newmtl") {
            iss >> currentMat;
        } else if (key == "map_Kd" && !currentMat.empty()) {
            std::string texFile;
            while (iss >> texFile) {}
            size_t p = texFile.find_last_of("/\\");
            std::string name = (p == std::string::npos) ? texFile : texFile.substr(p + 1);
            texMap[currentMat] = loadTexture("assets/models/" + name, 8);
        }
    }
    return texMap;
}

LoadedOBJModel loadOBJWithMaterials(const std::string& relPath, float targetSize = 1.15f) {
    LoadedOBJModel model;
    std::string path = resolvePath(relPath);
    std::ifstream file(path);
    if (!file.is_open()) return model;

    std::vector<vec3> positions;
    std::vector<std::pair<float, float>> texCoords;
    std::string currentMat = "default";
    std::string mtlRef;

    struct RawFace {
        std::string mat;
        std::vector<std::pair<int, int>> pairs;
    };
    std::vector<RawFace> rawFaces;

    std::string line;
    while (std::getline(file, line)) {
        if (line.size() < 2) continue;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "mtllib") {
            iss >> mtlRef;
        } else if (prefix == "usemtl") {
            iss >> currentMat;
        } else if (prefix == "v") {
            float x, y, z;
            if (iss >> x >> y >> z) positions.push_back(vec3(x, y, z));
        } else if (prefix == "vt") {
            float u = 0.0f, v = 0.0f;
            if (iss >> u >> v) texCoords.push_back({u, v});
        } else if (prefix == "f") {
            RawFace rf;
            rf.mat = currentMat;
            std::string token;
            while (iss >> token) {
                size_t s1 = token.find('/');
                std::string vStr = (s1 == std::string::npos) ? token : token.substr(0, s1);
                std::string vtStr;
                if (s1 != std::string::npos) {
                    size_t s2 = token.find('/', s1 + 1);
                    vtStr = (s2 == std::string::npos) ? token.substr(s1 + 1) : token.substr(s1 + 1, s2 - s1 - 1);
                }
                if (!vStr.empty()) {
                    int vIdx = std::stoi(vStr);
                    vIdx = (vIdx < 0) ? (int)positions.size() + vIdx : vIdx - 1;
                    int vtIdx = -1;
                    if (!vtStr.empty()) {
                        vtIdx = std::stoi(vtStr);
                        vtIdx = (vtIdx < 0) ? (int)texCoords.size() + vtIdx : vtIdx - 1;
                    }
                    if (vIdx >= 0 && vIdx < (int)positions.size()) {
                        rf.pairs.push_back({vIdx, vtIdx});
                    }
                }
            }
            if (rf.pairs.size() >= 3) rawFaces.push_back(rf);
        }
    }

    if (positions.empty() || rawFaces.empty()) return model;

    vec3 minB = positions[0], maxB = positions[0];
    for (const auto& p : positions) {
        minB.x = std::min(minB.x, p.x); minB.y = std::min(minB.y, p.y); minB.z = std::min(minB.z, p.z);
        maxB.x = std::max(maxB.x, p.x); maxB.y = std::max(maxB.y, p.y); maxB.z = std::max(maxB.z, p.z);
    }

    float span = std::max({maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z, 0.0001f});
    float scaleFactor = targetSize / span;
    vec3 center((minB.x + maxB.x) * 0.5f, minB.y, (minB.z + maxB.z) * 0.5f);

    for (const auto& rf : rawFaces) {
        OBJFace face;
        face.material = rf.mat;
        for (const auto& pr : rf.pairs) {
            OBJVertex vtx;
            vtx.pos = (positions[pr.first] - center) * scaleFactor;
            if (pr.second >= 0 && pr.second < (int)texCoords.size()) {
                vtx.u = texCoords[pr.second].first;
                vtx.v = texCoords[pr.second].second;
            }
            face.verts.push_back(vtx);
        }
        model.faces.push_back(face);
    }

    if (!mtlRef.empty()) model.materialTextures = loadMTLTextures(mtlRef);

    GLuint bobTex = loadTexture("assets/models/pendulum_bob_basecolor.jpg", 8);
    GLuint bodyTex = loadTexture("assets/models/pendulum_body_basecolor.jpg", 8);
    for (const auto& f : model.faces) {
        if (model.materialTextures.find(f.material) == model.materialTextures.end()) {
            model.materialTextures[f.material] =
                (f.material.find("bob") != std::string::npos) ? bobTex : bodyTex;
        }
    }

    model.loaded = true;
    return model;
}

void drawTableEdges(PolygonWithTexture& edgeFB, PolygonWithTexture& edgeLR,
                    float w, float d, float thickness, GLuint tex) {
    float hw = w * 0.5f;
    float hd = d * 0.5f;
    float zSides[2] = { hd, -hd };
    for (float z : zSides) {
        edgeFB.transformation(translate(mat4(1.0f), vec3(0.0f, -thickness, z)));
        edgeFB.setTexture(tex);
        edgeFB.draw();
    }

    float xSides[2] = { hw, -hw };
    for (float x : xSides) {
        edgeLR.transformation(
            translate(mat4(1.0f), vec3(x, -thickness, 0.0f)) *
            rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 1.0f, 0.0f))
        );
        edgeLR.setTexture(tex);
        edgeLR.draw();
    }
}

void drawTableLegs(PolygonWithTexture& legSide, float legSize, GLuint tex) {
    float legX[2] = { -1.95f, 1.95f };
    float legZ[2] = { -1.45f, 1.45f };
    float halfLeg = legSize * 0.5f;

    for (float x : legX) {
        for (float z : legZ) {
            vec3 base(x, -1.0f, z);

            legSide.transformation(translate(mat4(1.0f), base + vec3(0.0f, 0.0f, halfLeg)));
            legSide.setTexture(tex);
            legSide.draw();

            legSide.transformation(translate(mat4(1.0f), base + vec3(0.0f, 0.0f, -halfLeg)));
            legSide.setTexture(tex);
            legSide.draw();

            legSide.transformation(
                translate(mat4(1.0f), base + vec3(halfLeg, 0.0f, 0.0f)) *
                rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 1.0f, 0.0f))
            );
            legSide.setTexture(tex);
            legSide.draw();

            legSide.transformation(
                translate(mat4(1.0f), base + vec3(-halfLeg, 0.0f, 0.0f)) *
                rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 1.0f, 0.0f))
            );
            legSide.setTexture(tex);
            legSide.draw();
        }
    }
}

vec3 getDeckSlotPos(int index) {
    return vec3(-0.84f + index * 0.42f, 0.03f, 1.15f);
}

vec3 getPlaySlotPos(int playIndex) {
    return vec3(-1.2f + playIndex * 0.6f, 0.03f, 0.15f);
}

void setCameraMode(int mode) {
    cameraMode = mode;
    float stand = (mode == 1) ? 0.0f : 1.0f;

    if (mode == 1) {
        targetCamPos = vec3(0.0f, 6.2f, 0.01f);
        targetCamTarget = vec3(0.0f, 0.0f, 0.0f);
    } else {
        targetCamPos = vec3(0.0f, 2.5f, 5.2f);
        targetCamTarget = vec3(0.0f, 0.05f, 0.0f);
    }

    for (auto& c : deckCards) {
        c.targetStandFactor = stand;
    }
}

void triggerRandomDraw() {
    if (playedCount >= 5) return;

    std::vector<int> available;
    for (int i = 0; i < (int)deckCards.size(); ++i) {
        if (!deckCards[i].inPlay) available.push_back(i);
    }
    if (available.empty()) return;

    int pick = available[std::rand() % available.size()];
    deckCards[pick].inPlay = true;
    deckCards[pick].targetPos = getPlaySlotPos(playedCount++);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool k1 = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
    if (k1 && !key1WasPressed) setCameraMode(1);
    key1WasPressed = k1;

    bool k2 = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
    if (k2 && !key2WasPressed) setCameraMode(2);
    key2WasPressed = k2;

    bool kG = (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS);
    if (kG && !keyGWasPressed) triggerRandomDraw();
    keyGWasPressed = kG;
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Card Game Scene", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glEnable(GL_DEPTH_TEST);

    std::vector<std::pair<float, float>> roomUVs = {
        {0.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 4.0f}, {0.0f, 4.0f}
    };
    std::vector<std::pair<float, float>> quadUVs = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    };

    GLuint floorTex = loadTexture("assets/textures/floor.png", 1);
    GLuint wallTex  = loadTexture("assets/textures/wall.png", 2);
    GLuint tableTex = loadTexture("assets/textures/table.png", 3);
    GLuint deckTex  = loadTexture("assets/textures/zone_deck.png", 4);
    GLuint playTex  = loadTexture("assets/textures/zone_play.png", 5);
    GLuint oppTex   = loadTexture("assets/textures/zone_opp.png", 6);

    PolygonWithTexture floorPoly(makeQuad(16.0f, 16.0f), roomUVs);
    PolygonWithTexture backWall(makeVerticalQuad(16.0f, 5.5f), roomUVs);

    float tableW = 4.5f, tableD = 3.5f, tableThickness = 0.14f;
    PolygonWithTexture tableTop(makeQuad(tableW, tableD), quadUVs);
    PolygonWithTexture tableEdgeFB(makeVerticalQuad(tableW, tableThickness), quadUVs);
    PolygonWithTexture tableEdgeLR(makeVerticalQuad(tableD, tableThickness), quadUVs);

    float legSize = 0.22f;
    float legHeight = 1.0f - tableThickness;
    PolygonWithTexture legSide(makeVerticalQuad(legSize, legHeight), quadUVs);

    PolygonWithTexture deckZone(makeQuad(2.3f, 0.75f), quadUVs);
    PolygonWithTexture playZone(makeQuad(3.4f, 0.85f), quadUVs);
    PolygonWithTexture opponentZone(makeQuad(2.3f, 0.75f), quadUVs);

    GLuint posterTex = loadTexture("assets/poster/poster.png", 99);
    std::vector<std::pair<float, float>> posterUVs = {
        {0.0f,  0.0f}, {1.0f,  0.0f}, {1.0f,  0.52f},
        {0.89f, 0.64f}, {0.81f, 0.86f}, {0.625f,0.94f},
        {0.44f, 1.0f}, {0.225f,0.92f}, {0.0f,  0.96f}
    };
    PolygonWithTexture tornPoster(makeTornPosterQuad(2.35f, 1.65f), posterUVs);

    // Simple Gold Ingot
    Polygon goldTop({
        vec3(-0.22f, 0.13f,  0.09f), vec3( 0.22f, 0.13f,  0.09f),
        vec3( 0.26f, 0.13f, -0.09f), vec3(-0.26f, 0.13f, -0.09f)
    }, vec3(0.88f, 0.69f, 0.16f));
    Polygon goldFront({
        vec3(-0.30f, 0.01f,  0.15f), vec3( 0.30f, 0.01f,  0.15f),
        vec3( 0.22f, 0.13f,  0.09f), vec3(-0.22f, 0.13f,  0.09f)
    }, vec3(0.75f, 0.56f, 0.11f));
    Polygon goldBack({
        vec3(-0.26f, 0.13f, -0.09f), vec3( 0.26f, 0.13f, -0.09f),
        vec3( 0.30f, 0.01f, -0.15f), vec3(-0.30f, 0.01f, -0.15f)
    }, vec3(0.62f, 0.45f, 0.08f));
    Polygon goldLeft({
        vec3(-0.30f, 0.01f, -0.15f), vec3(-0.30f, 0.01f,  0.15f),
        vec3(-0.22f, 0.13f,  0.09f), vec3(-0.26f, 0.13f, -0.09f)
    }, vec3(0.68f, 0.50f, 0.09f));
    Polygon goldRight({
        vec3( 0.30f, 0.01f,  0.15f), vec3( 0.30f, 0.01f, -0.15f),
        vec3( 0.26f, 0.13f, -0.09f), vec3( 0.22f, 0.13f,  0.09f)
    }, vec3(0.58f, 0.42f, 0.07f));

    // Simple Candle without plate
    const int candleSides = 12;
    float candleRadius = 0.11f, candleHeight = 0.36f;
    std::vector<vec3> candleTopVerts;
    for (int i = 0; i < candleSides; ++i) {
        float ang = radians(i * (360.0f / candleSides));
        candleTopVerts.push_back(vec3(std::sin(ang) * candleRadius, candleHeight, std::cos(ang) * candleRadius));
    }
    Polygon candleWaxWall(makeVerticalQuad(0.062f, candleHeight), vec3(0.95f, 0.90f, 0.78f));
    Polygon candleWaxTop(candleTopVerts, vec3(0.99f, 0.96f, 0.87f));
    Polygon candleWick(makeVerticalQuad(0.02f, 0.065f), vec3(0.15f, 0.12f, 0.10f));

    // Ready-made 3D Pendulum
    LoadedOBJModel pendulumModel = loadOBJWithMaterials("assets/models/pendulum.obj", 1.15f);
    Polygon fallbackModelPoly(makeVerticalQuad(0.55f, 0.65f), vec3(0.42f, 0.52f, 0.64f));

    // 5 Deck Cards
    for (int i = 0; i < 5; ++i) {
        Card c;
        c.id = i;
        c.currentPos = getDeckSlotPos(i);
        c.targetPos = c.currentPos;
        c.textureID = loadTexture("assets/cards/card" + std::to_string(i + 1) + ".png", i + 1);
        deckCards.push_back(c);
    }
    PolygonWithTexture cardMesh(makeQuad(0.34f, 0.48f), quadUVs);

    mat4 projection = perspective(radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        processInput(window);

        float lerpSpeed = std::min(1.0f, dt * 6.0f);
        cameraPos = mix(cameraPos, targetCamPos, lerpSpeed);
        cameraTarget = mix(cameraTarget, targetCamTarget, lerpSpeed);

        for (auto& c : deckCards) {
            c.currentPos = mix(c.currentPos, c.targetPos, lerpSpeed);
            c.standFactor = mix(c.standFactor, c.targetStandFactor, lerpSpeed);
        }

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(value_ptr(projection));

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(value_ptr(lookAt(cameraPos, cameraTarget, cameraUp)));

        // Room & Poster
        floorPoly.transformation(translate(mat4(1.0f), vec3(0.0f, -1.0f, 0.0f)));
        floorPoly.setTexture(floorTex);
        floorPoly.draw();

        backWall.transformation(translate(mat4(1.0f), vec3(0.0f, -1.0f, -6.0f)));
        backWall.setTexture(wallTex);
        backWall.draw();

        tornPoster.transformation(translate(mat4(1.0f), vec3(-2.35f, 0.02f, -5.95f)));
        tornPoster.setTexture(posterTex);
        tornPoster.draw();

        // Table & Zones
        tableTop.transformation(mat4(1.0f));
        tableTop.setTexture(tableTex);
        tableTop.draw();

        drawTableEdges(tableEdgeFB, tableEdgeLR, tableW, tableD, tableThickness, tableTex);
        drawTableLegs(legSide, legSize, tableTex);

        deckZone.transformation(translate(mat4(1.0f), vec3(0.0f, 0.01f, 1.15f)));
        deckZone.setTexture(deckTex);
        deckZone.draw();

        playZone.transformation(translate(mat4(1.0f), vec3(0.0f, 0.01f, 0.15f)));
        playZone.setTexture(playTex);
        playZone.draw();

        opponentZone.transformation(translate(mat4(1.0f), vec3(0.0f, 0.01f, -0.78f)));
        opponentZone.setTexture(oppTex);
        opponentZone.draw();

        // Gold Bar
        mat4 goldMat = translate(mat4(1.0f), vec3(-1.75f, 0.0f, 1.05f)) *
                       rotate(mat4(1.0f), radians(-22.0f), vec3(0.0f, 1.0f, 0.0f));
        goldTop.transformation(goldMat);   goldTop.draw();
        goldFront.transformation(goldMat); goldFront.draw();
        goldBack.transformation(goldMat);  goldBack.draw();
        goldLeft.transformation(goldMat);  goldLeft.draw();
        goldRight.transformation(goldMat); goldRight.draw();

        // Candle without plate
        vec3 candlePos(1.75f, 0.0f, 1.05f);
        for (int s = 0; s < candleSides; ++s) {
            float shade = (s % 2 == 0) ? 1.0f : 0.94f;
            candleWaxWall.color = vec3(0.95f * shade, 0.90f * shade, 0.78f * shade);
            mat4 wMat = translate(mat4(1.0f), candlePos) *
                        rotate(mat4(1.0f), radians(s * (360.0f / candleSides)), vec3(0.0f, 1.0f, 0.0f)) *
                        translate(mat4(1.0f), vec3(0.0f, 0.0f, candleRadius * 0.96f));
            candleWaxWall.transformation(wMat);
            candleWaxWall.draw();
        }
        candleWaxTop.transformation(translate(mat4(1.0f), candlePos));
        candleWaxTop.draw();
        candleWick.transformation(translate(mat4(1.0f), candlePos + vec3(0.0f, candleHeight, 0.0f)));
        candleWick.draw();

        // Pendulum OBJ Model
        mat4 modelMat = translate(mat4(1.0f), vec3(1.75f, 0.01f, -1.15f));
        if (pendulumModel.loaded && !pendulumModel.faces.empty()) {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glMultMatrixf(value_ptr(modelMat));
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);

            GLuint lastTex = 0;
            for (const auto& f : pendulumModel.faces) {
                GLuint tex = pendulumModel.materialTextures[f.material];
                if (tex != lastTex) {
                    glBindTexture(GL_TEXTURE_2D, tex);
                    lastTex = tex;
                }
                glBegin(GL_TRIANGLE_FAN);
                for (const auto& v : f.verts) {
                    glTexCoord2f(v.u, v.v);
                    glVertex3f(v.pos.x, v.pos.y, v.pos.z);
                }
                glEnd();
            }

            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glPopMatrix();
        } else {
            fallbackModelPoly.transformation(modelMat);
            fallbackModelPoly.draw();
        }

        // Deck Cards
        for (const auto& c : deckCards) {
            vec3 pos = c.currentPos + vec3(0.0f, 0.24f * c.standFactor, 0.0f);
            mat4 mCard = rotate(translate(mat4(1.0f), pos), radians(90.0f * c.standFactor), vec3(1.0f, 0.0f, 0.0f));
            cardMesh.transformation(mCard);
            cardMesh.setTexture(c.textureID);
            cardMesh.draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}