/*
 * Based on timer query code by Jake Ryan https://gist.github.com/JuanDiegoMontoya/2e19a8a894b8df189d75fe4a9e5d8319
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

#include "FragCountQueryAsync.h"
#include "gl_errors.hpp"
#include <cassert>
#include <iostream>

FragCountQueryAsync::FragCountQueryAsync(uint32_t N)
  : capacity_(N)
{
  assert(capacity_ > 0);
  queries = new GLuint[capacity_];
  glGenQueries(capacity_, queries);
  GL_ERRORS();
}

FragCountQueryAsync::~FragCountQueryAsync()
{
  glDeleteQueries(capacity_, queries);
  delete[] queries;
}


FragCountQueryAsync::FragCountQueryAsync(const FragCountQueryAsync & original) : start_(original.start_), capacity_(original.capacity_) {
    queries = new GLuint[original.capacity_];
    glGenQueries(capacity_, queries);
    GL_ERRORS();
}

void FragCountQueryAsync::StartQuery()
{
  // begin a query if there is at least one inactive
  if (count_ < capacity_)
  {
    glBeginQuery(GL_SAMPLES_PASSED, queries[start_]);
    count_++;
  }
    GL_ERRORS();
}

void FragCountQueryAsync::EndQuery()
{
    if(count_ > 0) {
        //end same query
        glEndQuery(GL_SAMPLES_PASSED);
        start_ = (start_ + 1) % capacity_; // wrap
        GL_ERRORS();
    }
}

std::optional<GLuint> FragCountQueryAsync::most_recent_query()
{
  // return nothing if there is no active query
  if (count_ == 0)
  {
    return std::nullopt;
  }

  // get the index of the oldest query
  uint32_t index = (start_ + capacity_ - count_) % capacity_;
  uint32_t check_count = 1;

  GLint startResultAvailable{};
  glGetQueryObjectiv(queries[index], GL_QUERY_RESULT_AVAILABLE, &startResultAvailable);
  if(startResultAvailable) {
//      std::cout << index << " available\n";

      //at least one is available
      while(check_count < count_) { //stop if we check all possible ones
          index = (index + 1) % capacity_;
          glGetQueryObjectiv(queries[index], GL_QUERY_RESULT_AVAILABLE, &startResultAvailable);
          if(!startResultAvailable) {
//              std::cout << index << " unavailable\n";
              index = (index - 1) % capacity_; //if not available, go back to the most recent available one
              break;
          } else {
//              std::cout << index << " available\n";
          }
          check_count++;
      } //if loop guard breaks, all were ready, and index is the most recent query, which was ready

//      if(check_count ==count_) { //DEBUG
//          std::cout << "all the way finished\n";
//      } else {
//          std::cout << "partially finished\n";
//      }
  } else {
//      std::cout << index << " oldest not finished\n"; //DEBUG
      return std::nullopt; //oldest is not finished
  }


  // pop query and retrieve result
  count_--;
  GLuint result{};
//    std::cout << index << " taken\n";
  glGetQueryObjectuiv(queries[index], GL_QUERY_RESULT, &result);
    GL_ERRORS();
  return result;
}

GLuint FragCountQueryAsync::wait_for_query() {
    uint32_t index = (capacity_ + start_ - 1) % capacity_; //previous query
    GLuint result;
    glGetQueryObjectuiv(queries[index], GL_QUERY_RESULT, &result);
    count_ = 0; //all previous queries now useless, newest query popped
    GL_ERRORS();
    return result;
}

