//
// Created by legal on 02/02/2025.
//

#ifndef COLOR_STATS_COLORS_HPP
#define COLOR_STATS_COLORS_HPP

void inline error_section   (){ printf("\033[1;31m"); }
void inline warning_section (){ printf("\033[1;33m"); }
void inline info_section    (){ printf("\033[0;32m"); }
void inline debug_section   (){ printf("\033[0;35m"); }
void inline reset_section   (){ printf("\033[0m");    }

#endif //COLOR_STATS_COLORS_HPP
