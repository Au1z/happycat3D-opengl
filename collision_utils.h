#pragma once
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>
#include <cmath>

glm::vec3 ClosestPtPointTriangle(const glm::vec3& p,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;

    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return a;

    glm::vec3 bp = p - b;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0)
        return a + ab * (d1 / (d1 - d3));

    glm::vec3 cp = p - c;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0)
        return a + ac * (d2 / (d2 - d6));

    float va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0)
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

bool TestSphereTriangle(const glm::vec3& center, float radius,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
    glm::vec3& outP)
{
    outP = ClosestPtPointTriangle(center, a, b, c);
    return glm::length2(center - outP) <= radius * radius;
}

bool MollerTrumbore(const glm::vec3& rayOrig, const glm::vec3& rayDir,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    float& t, float& u, float& v)
{
    const float EPS = 1e-6;
    glm::vec3 e1 = v1 - v0;
    glm::vec3 e2 = v2 - v0;
    glm::vec3 p = glm::cross(rayDir, e2);
    float det = glm::dot(e1, p);
    if (fabs(det) < EPS) return false;

    float inv = 1.0f / det;
    glm::vec3 tvec = rayOrig - v0;
    u = glm::dot(tvec, p) * inv;
    if (u < 0 || u > 1) return false;

    glm::vec3 q = glm::cross(tvec, e1);
    v = glm::dot(rayDir, q) * inv;
    if (v < 0 || u + v > 1) return false;

    t = glm::dot(e2, q) * inv;
    return t > EPS;
}
