// 第 29.4 节：同一盘棋的四种编码：整盘 ASCII、每格 4 位、起点终点、合法着法序号
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using Board = std::array<char, 64>;

struct Move {
    int from = 0;
    int to = 0;
    bool operator==(const Move&) const = default;
};

int file_of(int square) { return square % 8; }
int rank_of(int square) { return square / 8; }
bool on_board(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}
bool is_white(char piece) { return piece >= 'A' && piece <= 'Z'; }
bool is_black(char piece) { return piece >= 'a' && piece <= 'z'; }

void add_if(const Board& board, int from, int to_file, int to_rank,
            bool white_to_move, std::vector<Move>& moves) {
    if (!on_board(to_file, to_rank)) return;
    const int to = to_rank * 8 + to_file;
    const char target = board[static_cast<std::size_t>(to)];
    if (target == '.') {
        moves.push_back({from, to});
    } else if (is_white(target) != white_to_move) {
        moves.push_back({from, to});        // 吃对方的子
    }
}

// 只生成“按走子规则能到”的着法：不处理将军、王车易位、吃过路兵和升变。
// 对本节的编码演示够用，因为样本着法都不依赖这些规则。
std::vector<Move> pseudo_legal(const Board& board, bool white_to_move) {
    std::vector<Move> moves;
    for (int from = 0; from < 64; ++from) {
        const char piece = board[static_cast<std::size_t>(from)];
        if (piece == '.') continue;
        if (is_white(piece) != white_to_move) continue;
        const char upper = static_cast<char>(piece >= 'a' ? piece - 'a' + 'A' : piece);
        const int file = file_of(from);
        const int rank = rank_of(from);

        if (upper == 'P') {
            const int dir = white_to_move ? 8 : -8;
            const int start_rank = white_to_move ? 1 : 6;
            const int one = from + dir;
            if (on_board(file, rank + (white_to_move ? 1 : -1))) {
                if (board[static_cast<std::size_t>(one)] == '.') {
                    moves.push_back({from, one});
                    const int two = from + 2 * dir;
                    if (rank == start_rank && board[static_cast<std::size_t>(two)] == '.') {
                        moves.push_back({from, two});
                    }
                }
                for (const int df : {-1, 1}) {
                    const int to_file = file + df;
                    const int to_rank = rank + (white_to_move ? 1 : -1);
                    if (!on_board(to_file, to_rank)) continue;
                    const int to = to_rank * 8 + to_file;
                    const char target = board[static_cast<std::size_t>(to)];
                    if (target != '.' && is_white(target) != white_to_move) {
                        moves.push_back({from, to});
                    }
                }
            }
            continue;
        }

        if (upper == 'N') {
            for (const auto [dr, df] : {std::pair{2, 1}, std::pair{2, -1}, std::pair{-2, 1},
                                        std::pair{-2, -1}, std::pair{1, 2}, std::pair{1, -2},
                                        std::pair{-1, 2}, std::pair{-1, -2}}) {
                add_if(board, from, file + df, rank + dr, white_to_move, moves);
            }
            continue;
        }

        if (upper == 'K') {
            for (const auto [dr, df] : {std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1},
                                        std::pair{0, -1}, std::pair{1, 1}, std::pair{1, -1},
                                        std::pair{-1, 1}, std::pair{-1, -1}}) {
                add_if(board, from, file + df, rank + dr, white_to_move, moves);
            }
            continue;
        }

        // 象、车、后：沿方向一直走到挡住为止。
        std::vector<std::pair<int, int>> directions;
        if (upper == 'B' || upper == 'Q') {
            directions.insert(directions.end(), {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}});
        }
        if (upper == 'R' || upper == 'Q') {
            directions.insert(directions.end(), {{1, 0}, {-1, 0}, {0, 1}, {0, -1}});
        }
        for (const auto [dr, df] : directions) {
            int r = rank + dr;
            int f = file + df;
            while (on_board(f, r)) {
                const int to = r * 8 + f;
                const char target = board[static_cast<std::size_t>(to)];
                if (target == '.') {
                    moves.push_back({from, to});
                } else {
                    if (is_white(target) != white_to_move) moves.push_back({from, to});
                    break;
                }
                r += dr;
                f += df;
            }
        }
    }
    return moves;
}

void apply(Board& board, const Move& move) {
    board[static_cast<std::size_t>(move.to)] = board[static_cast<std::size_t>(move.from)];
    board[static_cast<std::size_t>(move.from)] = '.';
}

int parse_square(const std::string& text) {
    return (text[1] - '1') * 8 + (text[0] - 'a');
}

constexpr int bits_for(std::size_t choices) {
    if (choices <= 1) return 0;
    return static_cast<int>(std::bit_width(static_cast<unsigned>(choices - 1)));
}

static_assert(bits_for(1) == 0);     // 强制着法不需要位
static_assert(bits_for(20) == 5);    // 开局 20 种着法
static_assert(bits_for(218) == 8);   // 书中举的最坏情况

struct BitWriter {
    std::vector<bool> bits;
    void append(unsigned value, int count) {
        for (int i = count - 1; i >= 0; --i) {
            bits.push_back(((value >> i) & 1u) != 0);
        }
    }
    std::size_t size() const { return bits.size(); }
};

struct BitReader {
    const std::vector<bool>& bits;
    std::size_t pos = 0;
    unsigned read(int count) {
        unsigned value = 0;
        for (int i = 0; i < count; ++i) {
            value = (value << 1) | (bits[pos] ? 1u : 0u);
            ++pos;
        }
        return value;
    }
};

int main() {
    Board board = {
        'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R',
        'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',
        'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r',
    };

    const std::vector<std::pair<std::string, std::string>> game = {
        {"e2", "e4"}, {"e7", "e5"}, {"f1", "c4"}, {"b8", "c6"},
        {"d1", "h5"}, {"g8", "f6"}, {"h5", "f7"},
    };

    std::vector<int> chosen_index;
    std::vector<int> bits_per_move;
    BitWriter packed;
    bool white_to_move = true;

    for (const auto& [from_text, to_text] : game) {
        const auto moves = pseudo_legal(board, white_to_move);
        const Move chosen{parse_square(from_text), parse_square(to_text)};
        const auto it = std::find(moves.begin(), moves.end(), chosen);
        if (it == moves.end()) {
            std::cout << "样本着法不在生成的着法表中: " << from_text << to_text << '\n';
            return 1;
        }
        const int index = static_cast<int>(it - moves.begin());
        const int count = bits_for(moves.size());
        packed.append(static_cast<unsigned>(index), count);
        chosen_index.push_back(index);
        bits_per_move.push_back(count);
        std::cout << from_text << to_text << ": " << moves.size()
                  << " 种着法, 序号 " << index << " 用 " << count << " bit\n";
        apply(board, chosen);
        white_to_move = !white_to_move;
    }

    // 读回来，确认每个序号都能还原成原来那一步。
    BitReader reader{packed.bits};
    bool round_trip = true;
    for (std::size_t i = 0; i < game.size(); ++i) {
        const unsigned index = reader.read(bits_per_move[i]);
        if (static_cast<int>(index) != chosen_index[i]) round_trip = false;
    }

    const std::size_t half_moves = game.size();
    std::cout << "\n" << half_moves << " 个半回合的总账:\n";
    std::cout << "  整盘 ASCII（每格 2 字节）: " << 128 * half_moves << " B\n";
    std::cout << "  每格 4 bit（3 位棋子 + 1 位颜色）: " << 32 * half_moves << " B\n";
    std::cout << "  只存起点和终点（各 6 bit）: " << 12 * half_moves << " bit = "
              << (12 * half_moves + 7) / 8 << " B\n";
    std::cout << "  合法着法序号: " << packed.size() << " bit = "
              << (packed.size() + 7) / 8 << " B\n";
    std::cout << "  解码校验: " << (round_trip ? "ok" : "失败") << '\n';

    // 同样的 7 步，用固定 2 字节的 uint16 存起点终点，要 14 字节。
    std::cout << "  对照：每步用 uint16 存 12 bit，共 " << 2 * half_moves << " B\n";
}
