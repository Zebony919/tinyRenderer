#include <cmath>
#include <tuple>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

constexpr int width  = 100;
constexpr int height = 100;

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

void drawTriangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    // Vertices a, b, and c are now sorted in ascending order so point a is lowest in terms of the y-axis
    if (ay > by) {std::swap(ax, bx); std::swap(ay, by);}
    if (ay > cy) {std::swap(ax, cx); std::swap(ay, cy);}
    if (by > cy) {std::swap(bx, cx); std::swap(by, cy);}
    // Boundary A is the line from the lowest point to the highest point
    // So in this case that would be from point a (lowest) to point c (highest)

    int totalHeight = cy - ay;

    if (ay != by) { // Meaning that point a and b aren't horizontally parallel where there is no "bottom half" in a sense
        int segmentHeight = by - ay;

        for (int y = ay; y <= by; y++) {
            int x1 = ax + ((cx - ax) * (y - ay)) / totalHeight;
            int x2 = ax + ((bx - ax) * (y - ay)) / segmentHeight;
            framebuffer.set(x1, y, color); // Boundary A line
            framebuffer.set(x2, y, color); // Boundary B line
            // Both lines are being drawn simultaneously

            for (int x = std::min(x1, x2); x < std::max(x1, x2); x++) {
                framebuffer.set(x, y, color);
            } // Drawing a horizontal line between the two points who share the same y-axis
            // Aparently using the line function isn't good for some future reasons but would 
            // work for this specific case
        }
    }

    if (by != cy) { // Meaning point b and c aren't horizontal
        int segmentHeight = cy - by;

        for (int y = by; y <= cy; y++) {
            int x1 = ax + ((cx - ax) * (y - ay)) / totalHeight;
            int x2 = bx + ((cx - bx) * (y - by)) / segmentHeight;

            for (int x = std::min(x1, x2); x < std::max(x1, x2); x++) {
                framebuffer.set(x, y, color);
            }
        }
    }
    // This loop is doing practically the same thing as the other but for the top half of the triangle
}

/* 
   Orthographic projection: drops the z-coordinate and remaps x,y 
   from model space [-1,1] into screen pixel coordinates [0,width]/[0,height].
   Not true perspective yet, no camera, no depth-based scaling.
*/
std::tuple<int, int> project(vec3 v) {
    return { (v.x + 1.) * width/2,
             (v.y + 1.) * height/2};
}

int main(int argc, char** argv) {
    /*
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1; 
    }
    */

    // Model model(argv[1]);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    drawTriangle(2, 5, 20, 24, 50, 21, framebuffer, red);
    drawTriangle(10, 20, 54, 37, 88, 12, framebuffer, blue);
    drawTriangle(20, 5, 34, 77, 18, 92, framebuffer, green);

    /*  
        For each face, project its 3 vertices from 3D model space 
        to 2D screen space, then draw the 3 edges connecting them.
    
    for (int i = 0; i < model.nfaces(); i++) {
        auto [ax, ay] = project(model.vert(i, 0).xyz());
        auto [bx, by] = project(model.vert(i, 1).xyz());
        auto [cx, cy] = project(model.vert(i, 2).xyz());

        line(ax, ay, bx, by, framebuffer, red);
        line(bx, by, cx, cy, framebuffer, red);
        line(cx, cy, ax, ay, framebuffer, red);
    }

    */


    /*
        Separately, draw a white dot at every vertex position in the model.
    
    for (int i = 0; i < model.nverts(); i++) {
        vec3 v = model.vert(i).xyz();
        auto [x, y] = project(v);
        framebuffer.set(x, y, white);
    }

    */

    framebuffer.write_tga_file("framebuffer.tga");
    std::cout << "Rendered successfully, wrote framebuffer.tga\n";
    return 0;
}