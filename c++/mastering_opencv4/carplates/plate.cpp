#include "plate.hpp"

Plate::Plate() 
{
}

Plate::Plate(cv::Mat img, cv::Rect pos)
{
    plate_img = img;
    position = pos;
}

std::string Plate::str() 
{
    /* concat strings with the x coordinate value of chars*/
    std::string result = "";
    std::vector<int> order_index;
    std::vector<int> x_positions;
    for (int i = 0; i < chars_pos.size(); i++) {
        order_index.push_back(i);
        x_positions.push_back(chars_pos[i].x);
    }

    float min  = x_positions[0];
    int min_idx = 0;
    for (int i = 0; i < x_positions.size(); i++) {
        min = x_positions[i];
        min_idx = i;
        for (int j = 0; j < x_positions.size(); j++) {
            if (x_positions[j] < min) {
                min = x_positions[j];
                min_idx = j;
            }
        }
        int aux_i = order_index[i];
        int aux_min = order_index[min_idx];
        order_index[i] = aux_min;
        order_index[min_idx] = aux_i;

        float aux_ai = x_positions[i];
        float aux_xmin = x_positions[min_idx];
        x_positions[i] = aux_xmin;
        x_positions[min_idx] = aux_xi;
    }

    for (int i = 0; i < order_index.size(); i++) {
        result = result+chars[orderIndex[i]];
    }

    return result;
}
