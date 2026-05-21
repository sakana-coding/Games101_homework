//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"

void Scene::buildBVH()
{
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray& ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection& pos, float& pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum)
            {
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(const Ray& ray,
                  const std::vector<Object*>& objects,
                  float& tNear,
                  uint32_t& index,
                  Object** hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear)
        {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }

    return (*hitObject != nullptr);
}
// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray& ray, int depth) const
{
    Intersection pos = intersect(ray);
    if (!pos.happened)
        return {0.0, 0.0, 0.0};
    // 如果光线直接打到光源那么直接返回
    if (pos.m->hasEmission())
        return pos.m->getEmission();

    Intersection inter = pos;
    Vector3f wo = -ray.direction, p = pos.coords, N = pos.normal;
    float pdf_light = 0.0f;
    sampleLight(inter, pdf_light);
    Vector3f x = inter.coords, NN = inter.normal, emit = inter.emit;//光源的位置、法线、发出的颜色RGB
    Vector3f ws = normalize(x - p);
    //对光线作了起始点偏移
    Ray ray_ = Ray(p + N * EPSILON, ws);

    Intersection block = intersect(ray_);
    float dist_light = (x - p).norm();
    float dist_hit = (block.coords - p).norm();
    Vector3f L_dir, L_indir;
    //对光源采样然后蒙特卡洛积分的部分
    if (!(block.happened && dist_hit < dist_light - EPSILON))
    {
        L_dir = emit * pos.m->eval(wo, ws, N) * dotProduct(ws, N) * dotProduct(-ws, NN) /
                (x - p).norm() / (x - p).norm() / pdf_light;
    }

    Vector3f wi = normalize(pos.m->sample(wo, N));
    Ray reflect_ray = Ray(p + N * EPSILON, wi);
    Intersection reflect_inter = intersect(reflect_ray);
    //对反射的光线进行递归运算，包括漫反射和镜面反射
    if (reflect_inter.happened && !reflect_inter.m->hasEmission())
    {
        L_indir = castRay(reflect_ray, depth + 1) * pos.m->eval(wo, wi, N) * dotProduct(wi, N) /
                  pos.m->pdf(wi, wo, N) / RussianRoulette;
    }
    return L_dir + L_indir;
}
