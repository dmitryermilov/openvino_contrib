// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <ngraph_functions/utils/ngraph_helpers.hpp>
#include <ngraph_functions/builders.hpp>
#include <ie_precision.hpp>
#include <functional_test_utils/include/functional_test_utils/precision_utils.hpp>
#include <ngraph/type/element_type.hpp>

inline
std::shared_ptr<ngraph::Function> CreateMatMulTestNetwork() {
    ngraph::helpers::InputLayerType secondaryInputType = ngraph::helpers::InputLayerType::CONSTANT;
    auto netPrecision = InferenceEngine::Precision::FP32;
    std::map<std::string, std::string> additionalConfig;

    auto ngPrc = FuncTestUtils::PrecisionUtils::convertIE2nGraphPrc(netPrecision);
    auto params = ngraph::builder::makeParams(ngPrc, {{3, 2, 10, 10}});

    auto secondaryInput = ngraph::builder::makeInputLayer(ngPrc, secondaryInputType, {3, 2, 10, 20});
    auto paramOuts = ngraph::helpers::convert2OutputVector(
        ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));
    auto MatMul = std::dynamic_pointer_cast<ngraph::opset3::MatMul>(
        ngraph::builder::makeMatMul(paramOuts[0], secondaryInput, false, false));
    ngraph::ResultVector results{std::make_shared<ngraph::opset1::Result>(MatMul)};
    return std::make_shared<ngraph::Function>(results, params, "MatMul");
}

class SuperDummyOp : public ngraph::op::Op {
 public:
    inline static constexpr type_info_t type_info{"SuperOperation", 0};
    const type_info_t& get_type_info() const override {
        return type_info;
    }

    std::shared_ptr<ngraph::Node>
    clone_with_new_inputs(const ngraph::OutputVector& new_args) const override {
        check_new_args_count(this, new_args);
        return std::make_shared<SuperDummyOp>(new_args.at(0), new_args.at(1));
    }

    void validate_and_infer_types() override {
        ngraph::element::Type result_et;

        NODE_VALIDATION_CHECK(
            this,
            ngraph::element::Type::merge(result_et, get_input_element_type(0), get_input_element_type(1)),
            "Arguments do not have the same element type (arg0 element type: ",
            get_input_element_type(0),
            ", arg1 element type: ",
            get_input_element_type(1),
            ").");

        const auto& A_partial_shape = get_input_partial_shape(0);
        const auto& B_partial_shape = get_input_partial_shape(1);

        if (A_partial_shape.rank().is_static() && B_partial_shape.rank().is_static()) {
            ngraph::PartialShape output_shape;
            set_output_type(0, result_et, A_partial_shape);
        } else {
            set_output_type(0, result_et, ngraph::PartialShape::dynamic());
        }
    }

    SuperDummyOp(const ngraph::Output<Node>& A,
                 const ngraph::Output<Node>& B)
        : ngraph::op::Op(ngraph::OutputVector{A, B}) {
        constructor_validate_and_infer_types();
    }
};

inline
std::shared_ptr<ngraph::Function> CreateSuperOperationTestNetwork() {
    ngraph::helpers::InputLayerType secondaryInputType = ngraph::helpers::InputLayerType::CONSTANT;
    auto netPrecision = InferenceEngine::Precision::FP32;
    std::map<std::string, std::string> additionalConfig;

    auto ngPrc = FuncTestUtils::PrecisionUtils::convertIE2nGraphPrc(netPrecision);
    auto params = ngraph::builder::makeParams(ngPrc, {{3, 2, 10, 10}});

    auto secondaryInput = ngraph::builder::makeInputLayer(ngPrc, secondaryInputType, {3, 2, 10, 20});
    auto paramOuts = ngraph::helpers::convert2OutputVector(
        ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));
    auto superOp = std::make_shared<SuperDummyOp>(paramOuts[0], secondaryInput);
    ngraph::ResultVector results{std::make_shared<ngraph::opset1::Result>(superOp)};
    return std::make_shared<ngraph::Function>(results, params, "SuperOperation");
}