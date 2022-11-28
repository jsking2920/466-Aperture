/*
 * Code by Jake Ryan https://gist.github.com/JuanDiegoMontoya/2e19a8a894b8df189d75fe4a9e5d8319
 *
 * License:
 * MIT License

    Copyright (c) 2021 Jake Ryan

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */
#include "GL.hpp"
#include <cstdint>
#include <optional>

// Async N-buffered timer query.
// Does not induce pipeline stalls.
// Useful for measuring performance of passes every frame without causing stalls.
// However, the results returned may be from multiple frames ago,
// and results are not guaranteed to be available.
// In practice, setting N to 5 should allow at least one query to be available.
class FragCountQueryAsync
{
public:
  FragCountQueryAsync(uint32_t N);
  ~FragCountQueryAsync();

  FragCountQueryAsync(const FragCountQueryAsync& original);
//  FragCountQueryAsync(FragCountQueryAsync&&) = delete;
//  FragCountQueryAsync& operator=(const FragCountQueryAsync&) = delete;
//  FragCountQueryAsync& operator=(FragCountQueryAsync&&) = delete;

  // begins or ends a query
  // always call End after Begin
  // never call Begin or End twice in a row
  void StartQuery();
  void EndQuery();

  // returns oldest query's result, if available
  // otherwise, returns std::nullopt
  [[nodiscard]] std::optional<GLuint> most_recent_query();

  //wait for newest query - used for single non-occlusion based tests like focal points
  [[nodiscard]] GLuint wait_for_query();

private:
  uint32_t start_{}; // next timer to be used for measurement
  uint32_t count_{}; // number of timers 'buffered', ie measurement was started by result not read yet
  uint32_t capacity_{};
  GLuint* queries{};
};