# TinyRenderer

A software rasterizer project, followed alongside the tinyRenderer project by Ssloy.
It walks you through how you can take an object file of vertices and faces and transform it into
realistic renderings of models with lighting and shading included.

# Some Renderings from my Software Rasterizer

(Drawing triangles using the bounding box and barycentric coordinates method with a random color applied)
<img width="799" height="800" alt="Screenshot 2026-09-07 125629" src="https://github.com/user-attachments/assets/422d99ad-e3ef-41d8-8bd1-451a53d7a6b5" />

(Added a light source for depth, where the brightness is determined by the dot product of the triangle and the light source's normal vector)
<img width="796" height="801" alt="Screenshot 2026-09-07 125735" src="https://github.com/user-attachments/assets/25bad541-2d79-4c59-81ef-28c41b43d0e9" />

(Calculating the normal vector per pixel using barycentric coordinates instead of reusing the same vector of the triangle for all fragments)
<img width="799" height="798" alt="Screenshot 2026-09-07 130051" src="https://github.com/user-attachments/assets/c27ba392-4792-4445-b64d-49f1d97d465a" />

(Using Tangent normal maps to add fake geometric texturing by tricking the light source and a Diffuse map to add colouring)
<img width="796" height="803" alt="Screenshot 2026-09-07 130407" src="https://github.com/user-attachments/assets/73b22286-6cd4-470d-a208-bc07a1479c51" />

(Some final renderings with shadow mapping applied and the Phong shading technique to apply more realistic lighting)
<img width="795" height="797" alt="Screenshot 2026-09-07 130647" src="https://github.com/user-attachments/assets/58f1cae0-1253-4b15-be20-c373d6bfe03c" />
<img width="796" height="798" alt="Screenshot 2026-09-07 130656" src="https://github.com/user-attachments/assets/c08f5364-7ed0-4d9e-a7d5-1b6b26675dfc" />
