#define _USE_MATH_DEFINES
#include <cmath>
#include <tuple>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"
#include "our_gl.h"

constexpr int width  = 800;
constexpr int height = 800;

extern mat<4, 4> ModelView, Perspective;
extern std::vector<double> zbuffer;

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax - bx) < std::abs(ay - by);

    if (steep) {
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax > bx) {
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    int y = ay;
    int ierror = 0;

    for (int x = ax; x <= bx; x++) {
        if (steep) {
            framebuffer.set(y, x, color);
        } else {
            framebuffer.set(x, y, color);
        }

        ierror += 2 * std::abs(by - ay);

        if (ierror > bx - ax) {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx - ax);
        }
    }
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return 0.5 * ((by - ay)*(bx + ax) + (cy - by)*(cx + bx) + (ay - cy)*(ax + cx));
    // Takes the sum of three trapezoids defined an edge, the x-axis, and two parallel lines going down
    // from each end of the edge to the x-axis. 
}

void drawTriangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage &framebuffer, TGAImage &zbuffer, TGAColor color) {
    // Vertices a, b, and c are now sorted in ascending order so point a is lowest in terms of the y-axis
    /*
    if (ay > by) {std::swap(ax, bx); std::swap(ay, by);}
    if (ay > cy) {std::swap(ax, cx); std::swap(ay, cy);}
    if (by > cy) {std::swap(bx, cx); std::swap(by, cy);}
    // Boundary A is the line from the lowest point to the highest point
    // So in this case that would be from point a (lowest) to point c (highest)
    // Use this ONLY for the scanline method
    */

    int bbminx = std::min(std::min(ax, bx), cx); // Lowest vertice
    int bbminy = std::min(std::min(ay, by), cy);
    int bbmaxx = std::max(std::max(ax, bx), cx); // Highest vertice
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

    if (total_area < 1) return;

    #pragma omp parallel for // Allows for parallel iteration where the compiler can split up the for loop amongst multiple cores
    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

            if (alpha < 0 || beta < 0 || gamma < 0) continue; // Negative barycentric coordinate
            // If its negative then the pixel is outside the triangle bound

            /*
            int aR = 255, aG = 0,   aB = 0;   // vertex A = red
            int bR = 0,   bG = 255, bB = 0;   // vertex B = green
            int cR = 0,   cG = 0,   cB = 255; // vertex C = blue

            unsigned char r = static_cast<unsigned char>(alpha * aR + beta * bR + gamma * cR);
            unsigned char g = static_cast<unsigned char>(alpha * aG + beta * bG + gamma * cG);
            unsigned char b = static_cast<unsigned char>(alpha * aB + beta * bB + gamma * cB);
            TGAColor newColor = {b, g, r, 255};
            // Allow triangle to be dynamically colors from the RGB range
            */

            unsigned char z = static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
            if (z <= zbuffer.get(x, y)[0]) continue;

            zbuffer.set(x, y, {z});
            framebuffer.set(x, y, color);
        }
    }
    

    /*
    int totalHeight = cy - ay;

    if (ay != by) { // Meaning that point a and b aren't horizontally parallel where there is no "bottom half" in a sense
        int segmentHeight = by - ay;

        for (int y = ay; y <= by; y++) {
            int x1 = ax + ((cx - ax) * (y - ay)) / totalHeight;
            int x2 = ax + ((bx - ax) * (y - ay)) / segmentHeight;

            
            for (int x = std::min(x1, x2); x < std::max(x1, x2); x++) {
                framebuffer.set(x, y, color);
            } // Drawing a horizontal line between the two points who share the same y-axis
            // Aparently using the line function isn't good for some future reasons but would 
            // work for this specific case
            // Solid Triangles
            

            for (int x = std::min(x1, x2); x < std::min(x1, x2) + 2; x++) {
                framebuffer.set(x, y, color);
            } // Wireframe triangles

            for (int x = std::max(x1, x2) - 2; x < std::max(x1, x2); x++) {
                framebuffer.set(x, y, color);
            }
            // These two for loops only render a border of the triangle
        }
    }
    
    if (by != cy) { // Meaning point b and c aren't horizontally parallel
        int segmentHeight = cy - by;

        for (int y = by; y <= cy; y++) {
            int x1 = ax + ((cx - ax) * (y - ay)) / totalHeight;
            int x2 = bx + ((cx - bx) * (y - by)) / segmentHeight;

            
            for (int x = std::min(x1, x2); x < std::max(x1, x2); x++) {
                framebuffer.set(x, y, color);
            } // Solid Triangles
            

            for (int x = std::min(x1, x2); x < std::min(x1, x2) + 2; x++) {
                framebuffer.set(x, y, color);
            } // Wireframe triangles

            for (int x = std::max(x1, x2) - 2; x < std::max(x1, x2); x++) {
                framebuffer.set(x, y, color);
            }
            // These two for loops only render a border of the triangle
        }
    }
    */
    // This loop is doing practically the same thing as the other but for the top half of the triangle
}

/* 
   Orthographic projection: drops the z-coordinate and remaps x,y 
   from model space [-1,1] into screen pixel coordinates [0,width]/[0,height].
   Not true perspective yet, no camera, no depth-based scaling.
*/
std::tuple<int, int, int> project(vec3 v) {
    return { (v.x + 1.) * width/2,
             (v.y + 1.) * height/2,
             (v.z + 1.) *   255./2 };
}

/*
vec3 rot(vec3 v) {
    constexpr double a = 11 * M_PI / 6;
    const mat<3,3> Ry = {{{std::cos(a), 0, std::sin(a)}, {0,1,0}, {-std::sin(a), 0, std::cos(a)}}};
    return Ry * v;
}
// This way of rotating is not the cheapest version as you are computing Ry for every pixel even 
// even though it doesn't change value since you are rotating every pixel by the same angle
*/

// Better version since you call it once before the loop in main
// These are actually the three rotation matrices which you can multiply to alter more than one axis
// or just use one to rotate around a fixed axis
mat<3, 3> rotateY(double radian) {
    return {{{std::cos(radian), 0, std::sin(radian)}, {0,1,0}, {-std::sin(radian), 0, std::cos(radian)}}};
}

mat<3, 3> rotateX(double radian) {
    return {{{1, 0, 0}, {0, std::cos(radian), -std::sin(radian)}, {0, std::sin(radian), std::cos(radian)}}};
}

mat<3, 3> rotateZ(double radian) {
    return {{{std::cos(radian), -std::sin(radian), 0}, {std::sin(radian), std::cos(radian), 0}, {0, 0, 1}}};
}

vec3 perspective(vec3 v) {
    constexpr double camera = 3.0;
    return v / (1 - v.z / camera);
}

struct RandomShader : IShader {
    const Model &model;
    TGAColor color = {};
    vec3 tri[3];

    RandomShader(const Model &m) : model(m) {
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert);
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.0};
        tri[vert] = gl_Position.xyz();
        return Perspective * gl_Position;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        return {false, color};
    }
};

struct PhongShader : IShader {
    const Model &model;
    TGAColor color = {};
    vec3 tri[3]; // Vertex position in eye/camera space
    vec3 varying_normals[3]; // Normal vector, normalized

    PhongShader(const Model &m) : model(m) {}

    virtual vec4 vertex(const int face, const int vert) { // Literally just returning a vertex from the model
        vec4 v = model.vert(face, vert);
        vec4 gl_position = ModelView * vec4{v.x, v.y, v.z, 1.0};
        tri[vert] = gl_position.xyz(); 

        vec3 n = model.normal(face, vert).xyz(); //  An average normal vector based on all faces that the vertex is apart of
        varying_normals[vert] = normalized((ModelView.invert_transpose() * vec4{n.x, n.y, n.z, 0.0}).xyz());
        // To transform normal vectors without making it not perpendicular, you have to use the inverse transpose of the same
        // matrix you use on all the other points. 

        return Perspective * gl_position; // Return the vector in clip space for the rasterize method
    }

    // Learn Fragment function
    virtual std::pair<bool, TGAColor> fragment(const vec3 bc) const {
        // Normal vector based on barycentric coordinates making the shading more smooth since every pixel will be calculated differently
        vec3 n = normalized(varying_normals[0] * bc.x + varying_normals[1] * bc.y + varying_normals[2] * bc.z);

        // Normal vector where every pixel has the same vector making it more blocking because it reveals the triangles. Every pixel in
        // the same triangle will be colored the same final color.
        // vec3 n = normalized(cross(tri[1] - tri[0], tri[2] - tri[0]));

        vec3 lightDir = normalized(vec3{1, 1, 1}); // Light Source
        double diffuse = std::max(0.0, n * lightDir); // Light Intensity based on how parallel the normal vector is to the light source

        vec3 reflectedLight = 2 * n * (n * lightDir) - lightDir;
        vec3 fragPosition = normalized(tri[0] * bc.x + tri[1] * bc.y + tri[2] * bc.z);
        vec3 view = normalized(vec3{0, 0, 0} - fragPosition);
        double specular = std::pow(std::max(0.0, reflectedLight.z), 35.0);

        double ambient = 0.3;
        double intensity = std::min(1.0, ambient + diffuse + specular);
        // Every pixel will have at least 30% of the baseColor and if its somewhat facing the light soucre, it will have an increase in intensity

        TGAColor baseColor = color;
        TGAColor shaded = color;
        for (int c = 0; c < 3; c++) {
            shaded[c] *= std::min(1.0, ambient + 0.75 * diffuse);
            shaded[c] *= std::min(1.0, ambient + 0.4 * diffuse + 0.9 * specular);
        }

        return {false, shaded};
    }
};

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1; 
    }
    

    // Draw Triangle method
    Model model(argv[1]);
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE); 
    
    constexpr double radians = 13 * M_PI / 6;
    const mat<3, 3> Ry = rotateY(radians);

    for (int i = 0; i < model.nfaces(); i++) {
        auto [ax, ay, az] = project(perspective(Ry * model.vert(i, 0).xyz()));
        auto [bx, by, bz] = project(perspective(Ry * model.vert(i, 1).xyz()));
        auto [cx, cy, cz] = project(perspective(Ry * model.vert(i, 2).xyz()));

        TGAColor rnd;
        for (int c=0; c<3; c++) rnd[c] = std::rand()%255;

        drawTriangle(ax, ay, az, bx, by, bz, cx, cy, cz, framebuffer, zbuffer, rnd);
    }

    // Rasterize method
    const vec3 eye{1, 0, 2}; // camera position
    constexpr vec3 center{0, 0, 0}; // camera direction
    constexpr vec3 up{0, 1, 0}; // camera vertical orientation 

    lookat(eye, center, up);
    init_perspective(norm(eye - center));
    init_viewport(width/16, height/16, width * 7/8, height * 7/8);
    init_zbuffer(width, height);

    TGAImage framebuffer2(width, height, TGAImage::RGB);
    TGAImage actualZbuffer3(width, height, TGAImage::GRAYSCALE);
    std::vector<double> zbuffer2(width * height, -std::numeric_limits<double>::max());

    // Change Background of TGA file by filling the entire scene before rendering
    TGAColor backgroundColor = {250, 250, 250, 255}; // dark gray, BGRA order remember
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            framebuffer2.set(x, y, backgroundColor);
        }
    }

    for (int m = 1; m < argc; m++) {
        Model model(argv[m]);
        PhongShader shader(model);

        for (int f = 0; f < model.nfaces(); f++) {
            shader.color = {    250, 150, 150, 255  }; // Base Color

            Triangle clip = { shader.vertex(f, 0),
                              shader.vertex(f, 1),
                              shader.vertex(f, 2) };

            rasterize(clip, shader, zbuffer2, framebuffer2, actualZbuffer3);
        }
    }    

    /*
    // Rasterize method with rotation (moving model)
    int x_rotation = 1;
    int numOfFrames2 = 60;

    for (int frame = 0; frame < numOfFrames2; frame++) {
        double angle = (2 * M_PI * frame) / numOfFrames2; // full 360 degree orbit across all frames
        vec3 orbitingEye{ 3 * std::cos(angle), 0, 3 * std::sin(angle) };

        lookat(orbitingEye, center, up);
        init_perspective(norm(orbitingEye - center));

        TGAImage framebuffer4(width, height, TGAImage::RGB);
        std::vector<double> zbuffer3(width * height, -std::numeric_limits<double>::max());

        for (int m = 1; m < argc; m++) {
            Model model(argv[m]);
            PhongShader shader(model);

            for (int f = 0; f < model.nfaces(); f++) {
                shader.color = {    250, 150, 150, 255  }; // Ambient Color

                Triangle clip = { shader.vertex(f, 0),
                                shader.vertex(f, 1),
                                shader.vertex(f, 2) };

                rasterize(clip, shader, zbuffer3, framebuffer4, actualZbuffer3);
            }
        }

        char filename[64];
        std::sprintf(filename, "frame_%03d.tga", frame);
        framebuffer4.write_tga_file(filename);
    }
    */

    /*
    // Rotation of the model in real time
    int numOfFrames = 120;
    for (int frame = 0; frame <= numOfFrames; frame++) {
        double radians = (2 * M_PI * frame) / numOfFrames;
        const mat<3, 3> Ry = rotateY(radians);
        TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
        
        for (int i = 0; i < model.nfaces(); i++) {
            auto [ax, ay, az] = project(perspective(Ry * model.vert(i, 0).xyz()));
            auto [bx, by, bz] = project(perspective(Ry * model.vert(i, 1).xyz()));
            auto [cx, cy, cz] = project(perspective(Ry * model.vert(i, 2).xyz()));

            TGAColor rnd;
            for (int c=0; c<3; c++) rnd[c] = std::rand()%255;

            drawTriangle(ax, ay, az, bx, by, bz, cx, cy, cz, framebuffer, zbuffer, rnd);
        }

        char filename[64];
        std::sprintf(filename, "frame_%03d.tga", frame);
        zbuffer.write_tga_file(filename);
    }
    */
    

    /*  
        For each face, project its 3 vertices from 3D model space 
        to 2D screen space, then draw the 3 edges connecting them.
    */

    /*
        Separately, draw a white dot at every vertex position in the model.
    
    for (int i = 0; i < model.nverts(); i++) {
        vec3 v = model.vert(i).xyz();
        auto [x, y] = project(v);
        framebuffer.set(x, y, white);
    }

    */

    // Old Draw triangle method
    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    // New rasterize method
    framebuffer2.write_tga_file("framebuffer2.tga");
    actualZbuffer3.write_tga_file("actualZbuffer3.tga");
    

    std::cout << "Rendered successfully, wrote framebuffer.tga\n";
    return 0;
}