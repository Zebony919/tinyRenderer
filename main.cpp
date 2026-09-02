#define _USE_MATH_DEFINES
#include <cmath>
#include <tuple>
#include <algorithm>
#include <limits>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

constexpr int width  = 800;
constexpr int height = 800;
mat<4, 4> ModelView, Viewport, Perspective;

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

void rasterize(const vec4 clip[3], std::vector<double> &zbuffer, TGAImage &framebuffer, TGAImage &actualZbuffer3, const TGAColor color) {
    vec4 ndc[3] = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w }; // Normalized Device Coordinates
    vec2 screen[3] = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() }; // Screen Coordinates

    mat<3, 3> ABC = {{ {screen[0].x, screen[0].y, 1.0}, {screen[1].x, screen[1].y, 1.0}, {screen[2].x, screen[2].y, 1.0} }}; // Three vertices
    if (ABC.det() < 1) return;

    auto [bbminx, bbmaxx] = std::minmax({screen[0].x, screen[1].x, screen[2].x}); // Bounding box for triangle
    auto [bbminy, bbmaxy] = std::minmax({screen[0].y, screen[1].y, screen[2].y}); // defined by the bottom left and top right points

    #pragma omp parallel for
    for (int x = std::max<int>(bbminx, 0); x <= std::min<int>(bbmaxx, framebuffer.width() - 1); x++) {
        for (int y = std::max<int>(bbminy, 0); y <= std::min<int>(bbmaxy, framebuffer.height() - 1); y++) {
            vec3 bc = ABC.invert_transpose() * vec3{static_cast<double>(x), static_cast<double>(y), 1.0};

            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue; // Negative barycentric coordinate means vertex outside triangle

            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };
            if (z <= zbuffer[x + y * framebuffer.width()]) continue; // Depth check on whether current pixel is infront of already drawn pixel

            unsigned char z2 = static_cast<unsigned char>((z * 0.5 + 0.5) * 255);

            zbuffer[x + y * framebuffer.width()] = z;
            framebuffer.set(x, y, color);
            actualZbuffer3.set(x, y, {z2});
        }
    }
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

void viewport(const int x, const int y, const int w, const int h) {
    Viewport = {{{w/2.0, 0, 0, x + w/2.0}, {0, h/2.0, 0, y + h/2.0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
}

void perspective(const double f) {
    Perspective = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1/f, 1}}};
}

void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    vec3 n = normalized(eye - center);
    vec3 l = normalized((cross(up, n)));
    vec3 m = normalized((cross(n, l)));

    ModelView = mat<4, 4>{{{l.x, l.y, l.z, 0}, {m.x, m.y, m.z, 0}, {n.x, n.y, n.z, 0}, {0, 0, 0, 1}}} * // Camera Matrix
                mat<4, 4>{{{1, 0, 0, -center.x}, {0, 1, 0, -center.y}, {0, 0, 1, -center.z}, {0, 0, 0, 1}}}; // Translation Matrix
}

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
    constexpr vec3 eye{-1, 0, 2}; // camera position
    constexpr vec3 center{0, 0, 0}; // camera direction
    constexpr vec3 up{0, 1, 0}; // camera vertical orientation 

    lookat(eye, center, up);
    perspective(norm(eye - center));
    viewport(width/16, height/16, width * 7/8, height * 7/8);

    TGAImage framebuffer2(width, height, TGAImage::RGB);
    TGAImage actualZbuffer3(width, height, TGAImage::GRAYSCALE);
    std::vector<double> zbuffer2(width * height, -std::numeric_limits<double>::max());

    for (int m = 1; m < argc; m++) {
        Model model(argv[m]);

        for (int i = 0; i < model.nfaces(); i++) {
            vec4 clip[3];
            for (int d : {0, 1, 2}) {
                vec4 v =  model.vert(i, d);
                clip[d] = Perspective * ModelView * v;
            }

            TGAColor rnd;
            for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
            rasterize(clip, zbuffer2, framebuffer2, actualZbuffer3, rnd);
        }
    }

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