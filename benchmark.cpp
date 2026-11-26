//
// Created by AbhishekJalkhare on 25-11-2026.
//
// Usage:
//   Benchmark [--exe <path>] [--src <file>] [--dst <file>] [--runs N]
//             [--args "<extra Copy_Tool args>"] [--sweep]
//
// Copy_Tool must already be built, and the source file must already exist.
//
// What it does differently from a single-shot test:
//   * repeats each method N times and reports median + min
//   * alternates which method goes first, so neither one always gets the
//     "warm cache" advantage
//   * measures process start-up cost (Copy_Tool --help) so you can see how much
//     of Copy_Tool's time is just launching the process via std::system
//   * verifies the copy is correct (size + content) once
//   * --sweep tries a grid of chunk sizes x buffer counts

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

#ifdef _WIN32
constexpr const char* kNullDevice = "NUL";
#else
constexpr const char* kNullDevice = "/dev/null";
#endif

static std::string quotePath(const std::string& p)
{
    std::string escaped;
    for (char c : p)
    {
        if (c == '"') escaped += '\\';
        escaped += c;
    }
    return "\"" + escaped + "\"";
}

static std::string buildCommand(const std::string& exe, const std::string& args)
{
    std::string cmd = quotePath(exe) + " " + args;
#ifdef _WIN32
    // Outer quotes are required so cmd /c doesn't strip the inner ones
    cmd = "\"" + cmd + "\"";
#endif
    return cmd;
}

static double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

static double minimum(const std::vector<double>& v)
{
    return *std::min_element(v.begin(), v.end());
}

// returns seconds, or -1 on failure
static double timeFs(const std::string& src, const std::string& dst)
{
    std::error_code ec;
    fs::remove(dst, ec);
    ec.clear();
    const auto t0 = Clock::now();
    const bool ok = fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    const auto t1 = Clock::now();
    if (!ok || ec)
    {
        std::cerr << "std::filesystem::copy_file failed: " << ec.message() << '\n';
        return -1;
    }
    return std::chrono::duration<double>(t1 - t0).count();
}

static double timeCustom(const std::string& exe, const std::string& src,
                         const std::string& dst, const std::string& extra)
{
    std::error_code ec;
    fs::remove(dst, ec);
    const std::string cmd = buildCommand(
        exe, quotePath(src) + " " + quotePath(dst) + (extra.empty() ? "" : " " + extra));
    const auto t0 = Clock::now();
    const int rc = std::system(cmd.c_str());
    const auto t1 = Clock::now();
    if (rc != 0)
    {
        std::cerr << "Copy_Tool failed (exit code " << rc << ")\n";
        return -1;
    }
    return std::chrono::duration<double>(t1 - t0).count();
}

static double timeStartup(const std::string& exe)
{
    const std::string cmd = buildCommand(exe, std::string("--help > ") + kNullDevice);
    const auto t0 = Clock::now();
    (void)std::system(cmd.c_str());
    const auto t1 = Clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

static bool sameContent(const std::string& a, const std::string& b)
{
    std::error_code ec;
    if (fs::file_size(a, ec) != fs::file_size(b, ec) || ec) return false;
    std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
    std::vector<char> ba(1 << 20), bb(1 << 20);
    while (fa && fb)
    {
        fa.read(ba.data(), ba.size());
        fb.read(bb.data(), bb.size());
        if (fa.gcount() != fb.gcount()) return false;
        if (!std::equal(ba.begin(), ba.begin() + fa.gcount(), bb.begin())) return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    std::string exe = R"(C:\Users\AbhishekJalkhare\CLionProjects\Copy_Tool\cmake-build-release\Copy_Tool.exe)";
    std::string src = "../test/srcfile";
    std::string dst = "../test/dstfile";
    std::string extra;
    int runs = 5;
    bool sweep = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "--exe") exe = next();
        else if (a == "--src") src = next();
        else if (a == "--dst") dst = next();
        else if (a == "--args") extra = next();
        else if (a == "--runs") runs = std::max(1, std::atoi(next().c_str()));
        else if (a == "--sweep") sweep = true;
        else { std::cerr << "Unknown option: " << a << '\n'; return 1; }
    }

    std::error_code ec;
    if (!fs::exists(src, ec) || ec)
    {
        std::cerr << "Error: source file does not exist: " << src << '\n';
        return 1;
    }
    if (!fs::exists(exe, ec) || ec)
    {
        std::cerr << "Error: Copy_Tool executable not found: " << exe << '\n';
        return 1;
    }

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Source size: " << fs::file_size(src, ec) / (1024.0 * 1024.0) << " MiB, runs per config: "
              << runs << "\n";

    // ---- process start-up overhead ----
    std::vector<double> startup;
    for (int i = 0; i < 5; ++i) startup.push_back(timeStartup(exe));
    const double startupMed = median(startup);
    std::cout << "Process start-up overhead (median of --help): " << startupMed << " s\n\n";

    // ---- correctness check, once ----
    if (timeCustom(exe, src, dst, extra) < 0) return 1;
    if (!sameContent(src, dst))
    {
        std::cerr << "Error: Copy_Tool output differs from source!\n";
        return 1;
    }
    std::cout << "Correctness check: OK\n\n";

    if (sweep)
    {
        // fs baseline
        std::vector<double> fsT;
        for (int i = 0; i < runs; ++i) { double t = timeFs(src, dst); if (t < 0) return 1; fsT.push_back(t); }
        std::cout << "std::filesystem::copy_file  median " << median(fsT) << " s  min " << minimum(fsT) << " s\n\n";

        const std::vector<long long> chunks = {256LL << 10, 1LL << 20, 4LL << 20, 8LL << 20, 16LL << 20};
        const std::vector<int> bufs = {2, 4, 8, 16};

        std::cout << std::left << std::setw(12) << "chunk" << std::setw(10) << "buffers"
                  << std::setw(14) << "median (s)" << "min (s)\n";
        for (long long c : chunks)
            for (int b : bufs)
            {
                const std::string args = "-c " + std::to_string(c) + " -b " + std::to_string(b);
                std::vector<double> t;
                for (int i = 0; i < runs; ++i)
                {
                    double s = timeCustom(exe, src, dst, args);
                    if (s < 0) return 1;
                    t.push_back(s);
                }
                std::cout << std::left << std::setw(12) << (std::to_string(c >> 10) + " KiB")
                          << std::setw(10) << b << std::setw(14) << median(t) << minimum(t) << '\n';
            }
        return 0;
    }

    // ---- head-to-head, alternating order ----
    std::vector<double> fsT, customT;
    for (int i = 0; i < runs; ++i)
    {
        double f, c;
        if (i % 2 == 0)
        {
            f = timeFs(src, dst);
            c = timeCustom(exe, src, dst, extra);
        }
        else
        {
            c = timeCustom(exe, src, dst, extra);
            f = timeFs(src, dst);
        }
        if (f < 0 || c < 0) return 1;
        fsT.push_back(f);
        customT.push_back(c);
        std::cout << "run " << i + 1 << ": fs " << f << " s, Copy_Tool " << c << " s\n";
    }

    const double fsMed = median(fsT), cMed = median(customT);
    const double cAdj = std::max(0.0, cMed - startupMed);

    std::cout << "\nstd::filesystem::copy_file  median " << fsMed << " s  min " << minimum(fsT) << " s\n";
    std::cout << "Copy_Tool.exe               median " << cMed << " s  min " << minimum(customT) << " s\n";
    std::cout << "Copy_Tool.exe (minus start-up) median " << cAdj << " s\n";

    if (cMed < fsMed)
        std::cout << "Copy_Tool was faster by a factor of " << fsMed / cMed << '\n';
    else
        std::cout << "std::filesystem::copy_file was faster by a factor of " << cMed / fsMed << '\n';

    fs::remove(dst, ec);
    return 0;
}