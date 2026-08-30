#include <cmath>
#include <tuple>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

constexpr int width  = 800;
constexpr int height = 800;

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

void drawTriangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer) {
    // Vertices a, b, and c are now sorted in ascending order so point a is lowest in terms of the y-axis
    if (ay > by) {std::swap(ax, bx); std::swap(ay, by);}
    if (ay > cy) {std::swap(ax, cx); std::swap(ay, cy);}
    if (by > cy) {std::swap(bx, cx); std::swap(by, cy);}
    // Boundary A is the line from the lowest point to the highest point
    // So in this case that would be from point a (lowest) to point c (highest)

    int bbminx = std::min(std::min(ax, bx), cx); // Lowest vertice
    int bbminy = std::min(std::min(ay, by), cy);
    int bbmaxx = std::max(std::max(ax, bx), cx); // Highest vertice
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

    // if (total_area < 1) return; // Doesn't draw triangles less than 1 pixel in size

    #pragma omp parallel for // Allows for parallel iteration where the compiler can split up the for loop amongst multiple cores
    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

            if (alpha < 0 || beta < 0 || gamma < 0) continue; // Negative barycentric coordinate
            // If its negative then the pixel is outside the triangle bound

            int aR = 255, aG = 0,   aB = 0;   // vertex A = red
            int bR = 0,   bG = 255, bB = 0;   // vertex B = green
            int cR = 0,   cG = 0,   cB = 255; // vertex C = blue

            unsigned char r = static_cast<unsigned char>(alpha * aR + beta * bR + gamma * cR);
            unsigned char g = static_cast<unsigned char>(alpha * aG + beta * bG + gamma * cG);
            unsigned char b = static_cast<unsigned char>(alpha * aB + beta * bB + gamma * cB);
            TGAColor newColor = {b, g, r, 255};
            // Allow triangle to be dynamically colors from the RGB range

            framebuffer.set(x, y, newColor);
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
        }
    }

    if (by != cy) { // Meaning point b and c aren't horizontally parallel
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
    */
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
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1; 
    }
    

    Model model(argv[1]);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    int ax = 17, ay =  4, az =  13;
    int bx = 55, by = 39, bz = 128;
    int cx = 23, cy = 59, cz = 255;

    /*  
        For each face, project its 3 vertices from 3D model space 
        to 2D screen space, then draw the 3 edges connecting them.
    */
    
    for (int i = 0; i < model.nfaces(); i++) {
        auto [ax, ay] = project(model.vert(i, 0).xyz());
        auto [bx, by] = project(model.vert(i, 1).xyz());
        auto [cx, cy] = project(model.vert(i, 2).xyz());

        drawTriangle(ax, ay, bx, by, cx, cy, framebuffer);
    }



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