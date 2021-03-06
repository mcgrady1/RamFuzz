// Copyright 2016 The RamFuzz contributors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

class A {
public:
  int num = -12;
  A() : num(3) {}
};

class B {
public:
  int sum = 0;
  void f(int *p, A *a) { sum += bool(p) * a->num; }
  void g(double ***&p) {}
  void h1(void **p) {}
  void h2(const void **p) {}
  void h3(const void *const *p) {}
  void i1(const void *p) {}
  void i2(const int *p) {}
};
