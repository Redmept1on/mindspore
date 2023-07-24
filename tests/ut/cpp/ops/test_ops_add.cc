/**
 * Copyright 2023 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <vector>
#include <memory>
#include "common/common_test.h"
#include "ops/add.h"
#include "ir/dtype/type.h"
#include "abstract/dshape.h"
#include "utils/tensor_construct_utils.h"
#include "ir/primitive.h"
#include "abstract/abstract_value.h"
#include "utils/ms_context.h"

namespace mindspore {
namespace ops {
class TestAdd : public UT::Common {
 public:
  TestAdd() {}
  void SetUp() {
    auto context_ptr = MsContext::GetInstance();
    MS_EXCEPTION_IF_NULL(context_ptr);
    origin_device_target_ = context_ptr->get_param<std::string>(MS_CTX_DEVICE_TARGET);
  }
  void TearDown() {
    auto context_ptr = MsContext::GetInstance();
    if (context_ptr != nullptr) {
      context_ptr->set_param<std::string>(MS_CTX_DEVICE_TARGET, origin_device_target_);
    }
  }

 private:
  std::string origin_device_target_;
};

struct TestAddParams {
  ShapeVector x_shape;
  TypePtr x_type;
  ShapeVector y_shape;
  TypePtr y_type;
  ShapeVector out_shape;
  TypePtr out_type;
};

class TestAddOnece : public TestAdd, public testing::WithParamInterface<TestAddParams> {};

TEST_P(TestAddOnece, add_dyn_shape) {
  const auto &param = GetParam();
  auto x = std::make_shared<abstract::AbstractTensor>(param.x_type, param.x_shape);
  auto y = std::make_shared<abstract::AbstractTensor>(param.y_type, param.y_shape);
  auto expect = std::make_shared<abstract::AbstractTensor>(param.out_type, param.out_shape);
  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  auto add = std::make_shared<Add>();
  add->Init();
  auto prim = std::make_shared<Primitive>(kNameAdd);
  auto out_abstract = AddInfer(nullptr, prim, {x, y});
  ASSERT_NE(out_abstract, nullptr);
  ASSERT_TRUE(*out_abstract == *expect);
}

INSTANTIATE_TEST_CASE_P(TestAddGroup, TestAddOnece,
                        testing::Values(
                          TestAddParams{
                            ShapeVector{-1, -1},
                            kFloat32,
                            ShapeVector{2, 3},
                            kFloat32,
                            ShapeVector{-1, -1},
                            kFloat32,
                          },
                          TestAddParams{
                            ShapeVector{-2},
                            kFloat32,
                            ShapeVector{2, 3},
                            kFloat32,
                            ShapeVector{-2},
                            kFloat32,
                          }));
}  // namespace ops
}  // namespace mindspore
