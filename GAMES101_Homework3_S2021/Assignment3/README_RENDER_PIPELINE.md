# Assignment3 渲染管线与 Shader 梳理

这份文档按“数据从哪里来、经过哪些阶段、最后如何变成一张图”来解释本项目。

---

## 1. 先看整体流程

这份作业里的渲染流程可以概括成：

1. **加载模型**
2. **为每个三角形准备顶点位置 / 法线 / 纹理坐标**
3. **做坐标变换**
   - Model
   - View
   - Projection
   - 透视除法
   - Viewport 映射
4. **光栅化三角形**
   - 遍历包围盒
   - 判断像素是否在三角形内
   - 重心坐标插值各种属性
   - 深度测试
5. **执行 Fragment Shader**
   - normal / phong / texture / bump / displacement
6. **把最终颜色写入 framebuffer**
7. **framebuffer 转成 OpenCV 图像并保存**

---

## 2. 代码结构对应关系

### 主要入口
- `main.cpp`

负责：
- 读取模型
- 选择 shader
- 设置 `model/view/projection`
- 调用 `r.draw(TriangleList)`
- 输出图片

### 光栅器
- `rasterizer.cpp`
- `rasterizer.hpp`

负责：
- 把三角形变换到屏幕空间
- 做光栅化
- 插值属性
- 调用 fragment shader
- 写颜色和深度

### Shader 载荷
- `Shader.hpp`

其中最关键的是：

```cpp
struct fragment_shader_payload
{
    Eigen::Vector3f view_pos;
    Eigen::Vector3f color;
    Eigen::Vector3f normal;
    Eigen::Vector2f tex_coords;
    Texture* texture;
};
```

这表示：**fragment shader 需要的输入**。

### 纹理
- `Texture.hpp`

负责：
- 从图片读取颜色
- 根据 `(u, v)` 返回纹理颜色

---

## 3. 模型是如何进入渲染管线的

在 `main.cpp` 中：

1. 从 `spot_triangulated_good.obj` 读取网格
2. 每 3 个顶点组成一个 `Triangle`
3. 为每个顶点设置：
   - `setVertex`
   - `setNormal`
   - `setTexCoord`

所以每个三角形顶点携带的信息有：

- 顶点位置 position
- 法线 normal
- 纹理坐标 texcoord

---

## 4. 坐标变换阶段

在 `rasterizer::draw(std::vector<Triangle*>& TriangleList)` 里：

### 4.1 顶点位置变换

```cpp
Eigen::Matrix4f mvp = projection * view * model;
Eigen::Vector4f v[] = {mvp * t->v[0], mvp * t->v[1], mvp * t->v[2]};
```

这一步把模型空间顶点变到裁剪空间。

然后做：

```cpp
vec.x() /= vec.w();
vec.y() /= vec.w();
vec.z() /= vec.w();
```

这是 **透视除法**，得到 NDC。

再做 viewport 变换，把坐标映射到屏幕像素范围。

---

### 4.2 法线变换

```cpp
Eigen::Matrix4f inv_trans = (view * model).inverse().transpose();
```

法线不能直接乘 `model`，而应乘 **逆转置矩阵**。

然后：

```cpp
newtri.setNormal(i, n[i].head<3>());
```

得到 view space 下的法线。

---

### 4.3 view_pos 的来源

```cpp
std::array<Eigen::Vector4f, 3> mm{
    (view * model * t->v[0]),
    (view * model * t->v[1]),
    (view * model * t->v[2])};
```

再取前三维：

```cpp
viewspace_pos
```

然后传给：

```cpp
rasterize_triangle(newtri, viewspace_pos);
```

这说明 `payload.view_pos` 用的是 **view space 位置**，不是屏幕 `(x,y)`，也不是原始模型空间坐标。

---

## 5. 光栅化阶段在干什么

在 `rasterizer::rasterize_triangle(...)` 中：

### 5.1 求三角形包围盒

```cpp
left / right / below / top
```

只遍历包围盒内像素，减少工作量。

### 5.2 insideTriangle

判断当前像素是否真的在三角形内部。

### 5.3 重心坐标

```cpp
computeBarycentric2D(x, y, t.v)
```

得到：

- `alpha`
- `beta`
- `gamma`

它们用于插值任意顶点属性。

### 5.4 透视正确插值

你现在已经对这些量做了插值：

- `interpolated_color`
- `interpolated_normal`
- `interpolated_texcoords`
- `interpolated_shadingcoords`（其实就是 view_pos）
- `interpolated_z`

注意：这里不是简单线性插值，而是 **透视正确插值**。

通用形式是：

```cpp
attr = (alpha * a0 / w0 + beta * a1 / w1 + gamma * a2 / w2) * w_reciprocal
```

其中：

```cpp
w_reciprocal = 1 / (alpha / w0 + beta / w1 + gamma / w2)
```

### 5.5 深度测试

```cpp
if (interpolated_z < depth_buf[index])
```

如果当前片元更靠前：

- 更新颜色
- 更新深度

---

## 6. Fragment Shader 是怎么接上的

在光栅化阶段里，你会构造：

```cpp
fragment_shader_payload payload(
    interpolated_color,
    interpolated_normal.normalized(),
    interpolated_texcoords,
    texture ? &*texture : nullptr);

payload.view_pos = interpolated_shadingcoords;
```

然后调用：

```cpp
auto pixel_color = fragment_shader(payload);
```

这就是整个作业的核心：

> 光栅化阶段负责给 shader 提供“当前片元的属性”，  
> shader 负责根据这些属性算“当前片元最终颜色”。

---

## 7. 各个 Shader 的区别

---

### 7.1 `normal_fragment_shader`

作用：
- 不做真实光照
- 直接把法线可视化成颜色

公式本质：

```cpp
color = (normal + 1) / 2
```

用途：
- 检查法线方向是否正确
- 检查插值是否正确

---

### 7.2 `phong_fragment_shader`

作用：
- 使用 Phong 光照模型

输入：
- `payload.color`
- `payload.normal`
- `payload.view_pos`

输出由三部分组成：

#### Ambient

环境光：

```cpp
ka ⊙ Ia
```

#### Diffuse

漫反射：

```cpp
kd ⊙ (I / r^2) * max(0, n·l)
```

#### Specular

高光：

```cpp
ks ⊙ (I / r^2) * pow(max(0, n·h), p)
```

其中：

- `n`：法线
- `l`：光线方向
- `v`：观察方向
- `h`：半程向量 `normalize(l + v)`
- `⊙`：逐分量乘法，即 `cwiseProduct`

---

### 7.3 `texture_fragment_shader`

本质：

> **Phong 光照 + 纹理颜色**

区别在于 `kd` 不再取顶点颜色，而取：

```cpp
texture_color = texture->getColor(u, v)
kd = texture_color / 255.f
```

也就是说：
- 光照公式和 Phong 一样
- 但表面底色来自纹理

---

### 7.4 `bump_fragment_shader`

本质：

> **不改几何位置，只改法线**

思路：

1. 取原始法线 `n=(x,y,z)`
2. 构造切线 `t`
3. 求副切线 `b = n × t`
4. 组成 `TBN`
5. 用高度图估计：
   - `dU`
   - `dV`
6. 构造局部扰动法线：

```cpp
ln = (-dU, -dV, 1)
```

7. 转回 view space：

```cpp
n' = normalize(TBN * ln)
```

这个 shader 当前返回的是：

```cpp
result_color = normal
```

所以你看到的是“扰动后法线的可视化”，而不是最终带光照的 bump 成品图。

#### 关键理解

- **轮廓不变**
- **表面光照细节变化**

---

### 7.5 `displacement_fragment_shader`

本质：

> **既改法线，也改片元位置**

在 bump 的基础上，额外做：

```cpp
point = point + kn * normal * h(u, v)
```

然后再用新位置 `point` 和新法线 `normal` 进入 Phong 光照计算。

#### 与 bump 的区别

- `bump`：只让表面“看起来”凹凸
- `displacement`：真的让着色点位置发生位移

在你的实现里，由于这是屏幕空间光栅化后再做片元级位移，它并不会改变模型轮廓网格本身，但会改变光照计算使用的位置。

---

## 8. bump / displacement 为什么都要用 TBN

高度图提供的是纹理空间里的高度变化，
也就是在 `(u,v)` 参数域中的变化。

但光照是在 3D 空间里算的，所以必须把“纹理空间中的扰动”转换到 3D 空间。

`TBN` 就是这个变换桥梁：

- `T`：切线方向
- `B`：副切线方向
- `N`：法线方向

它把局部法线扰动 `ln` 变换到当前表面的真实空间方向。

---

## 9. 为什么 shader 输出的是最终颜色，而不是直接写插值颜色

因为插值出来的：

- color
- normal
- tex_coords
- view_pos

都只是 **shader 的输入**，不是最终显示结果。

最终颜色要结合：

- 光源位置
- 光源强度
- 法线方向
- 观察方向
- 纹理颜色
- 高度图

一起算出来。

所以流程是：

```text
插值属性 -> 组装 payload -> 调用 shader -> 得到最终颜色 -> 写 framebuffer
```

---

## 10. 这份作业里最容易混淆的几个坐标

### 屏幕坐标 `(x, y)`
- 当前正在写的像素位置
- 用于 `insideTriangle`
- 用于查深度缓存索引

### `payload.tex_coords`
- 纹理坐标 `(u, v)`
- 用于采样纹理 / 高度图

### `payload.view_pos`
- 当前片元在 **view space** 下的 3D 位置
- 用于计算光照方向、视线方向

### `payload.normal`
- 当前片元插值后的法线
- 通常要 `normalized()`

---

## 11. 一张简化流程图

```text
OBJ 模型
  ↓
Triangle 顶点数据(position / normal / texcoord)
  ↓
Model / View / Projection 变换
  ↓
屏幕空间三角形
  ↓
rasterize_triangle
  ├─ insideTriangle
  ├─ barycentric
  ├─ 插值 color / normal / texcoord / view_pos / z
  ├─ depth test
  └─ fragment_shader(payload)
          ├─ normal shader
          ├─ phong shader
          ├─ texture shader
          ├─ bump shader
          └─ displacement shader
  ↓
最终像素颜色
  ↓
framebuffer
  ↓
OpenCV 图片
  ↓
保存为 png
```

---

## 12. 你现在可以怎么用这份文档

如果你以后再看代码，推荐按这个顺序读：

1. `main.cpp`
   - 看 shader 怎么选
2. `rasterizer::draw`
   - 看顶点怎么变换
3. `rasterizer::rasterize_triangle`
   - 看像素级工作怎么做
4. `Shader.hpp`
   - 看 payload 里到底传了什么
5. 各个 fragment shader
   - 看每种效果到底改了哪里

---

## 13. 一句话总结

这份作业的本质是：

> **先把三角形光栅化成很多片元，再把每个片元需要的属性插值出来，交给不同的 fragment shader 去决定最终颜色。**

而不同 shader 的核心差异在于：

- 用什么做 `kd`
- 法线是否扰动
- 位置是否位移

