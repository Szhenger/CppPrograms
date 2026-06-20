#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <format>
#include <algorithm>
#include <random>

// OpenCV for image processing
#include <opencv2/opencv.hpp>
// LibTorch for Deep Learning
#include <torch/torch.h>

namespace fs = std::filesystem;

// Hyperparameters
constexpr int EPOCHS = 10;
constexpr int IMG_WIDTH = 30;
constexpr int IMG_HEIGHT = 30;
constexpr int NUM_CATEGORIES = 43;
constexpr float TEST_SIZE = 0.4f;

// ---------------------------------------------------------
// Model Definition
// ---------------------------------------------------------
struct TrafficNet : torch::nn::Module {
    TrafficNet() {
        // Python: Conv2D(filters=64, kernel_size=(3, 3), activation='relu')
        conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(3, 64, 3)));
        // Python: MaxPooling2D(pool_size=(3, 3))
        pool1 = register_module("pool1", torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions({3, 3})));
        
        // Python: Conv2D(filters=32, kernel_size=(2, 2), activation='softmax')
        conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 32, 2)));
        // Python: MaxPooling2D(pool_size=(2, 2))
        pool2 = register_module("pool2", torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions({2, 2})));
        
        // Flatten size calculation:
        // 30x30 -> Conv(3) -> 28x28 -> MaxPool(3) -> 9x9
        // 9x9 -> Conv(2) -> 8x8 -> MaxPool(2) -> 4x4
        // 32 channels * 4 * 4 = 512
        fc1 = register_module("fc1", torch::nn::Linear(32 * 4 * 4, 128));
        fc2 = register_module("fc2", torch::nn::Linear(128, NUM_CATEGORIES)); // Corrected from 10 to NUM_CATEGORIES
        dropout = register_module("dropout", torch::nn::Dropout(0.5));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(conv1->forward(x));
        x = pool1->forward(x);
        
        x = torch::softmax(conv2->forward(x), /*dim=*/1);
        x = pool2->forward(x);
        
        x = x.view({x.size(0), -1}); // Flatten
        
        x = torch::relu(fc1->forward(x));
        x = dropout->forward(x);     // Corrected: Dropout before final layer
        x = fc2->forward(x);
        
        return torch::log_softmax(x, /*dim=*/1);
    }

    torch::nn::Conv2d conv1{nullptr}, conv2{nullptr};
    torch::nn::MaxPool2d pool1{nullptr}, pool2{nullptr};
    torch::nn::Linear fc1{nullptr}, fc2{nullptr};
    torch::nn::Dropout dropout{nullptr};
};

// ---------------------------------------------------------
// Data Loading
// ---------------------------------------------------------
std::pair<torch::Tensor, torch::Tensor> load_data(const std::string& data_dir) {
    std::vector<torch::Tensor> images;
    std::vector<int64_t> labels;

    for (int category = 0; category < NUM_CATEGORIES; ++category) {
        fs::path path = fs::path(data_dir) / std::to_string(category);
        
        if (!fs::exists(path)) continue;

        for (const auto& entry : fs::directory_iterator(path)) {
            cv::Mat img = cv::imread(entry.path().string());
            if (img.empty()) continue;

            cv::Mat resized;
            cv::resize(img, resized, cv::Size(IMG_WIDTH, IMG_HEIGHT));
            
            // Convert OpenCV BGR to standard RGB
            cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
            
            // Normalize pixels to [0, 1]
            resized.convertTo(resized, CV_32FC3, 1.0f / 255.0f);

            // Convert cv::Mat to torch::Tensor (HWC to CHW format)
            torch::Tensor tensor_img = torch::from_blob(
                resized.data, {IMG_HEIGHT, IMG_WIDTH, 3}, torch::kFloat32
            ).clone();
            tensor_img = tensor_img.permute({2, 0, 1});

            images.push_back(tensor_img);
            labels.push_back(category);
        }
    }

    // Stack vectors into continuous tensors
    return {torch::stack(images), torch::tensor(labels, torch::kInt64)};
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: ./traffic data_directory [model.pt]\n";
        return EXIT_FAILURE;
    }

    std::string data_dir = argv[1];

    std::cout << "Loading data...\n";
    auto [images, labels] = load_data(data_dir);
    int64_t num_samples = images.size(0);
    std::cout << std::format("Loaded {} images.\n", num_samples);

    // Split data into training and testing sets
    int64_t test_count = static_cast<int64_t>(num_samples * TEST_SIZE);
    int64_t train_count = num_samples - test_count;

    // Shuffle indices for random split
    torch::Tensor indices = torch::randperm(num_samples);
    torch::Tensor train_indices = indices.slice(0, 0, train_count);
    torch::Tensor test_indices = indices.slice(0, train_count, num_samples);

    torch::Tensor x_train = images.index_select(0, train_indices);
    torch::Tensor y_train = labels.index_select(0, train_indices);
    torch::Tensor x_test = images.index_select(0, test_indices);
    torch::Tensor y_test = labels.index_select(0, test_indices);

    // Create DataLoaders
    auto train_dataset = torch::data::datasets::TensorDataset(x_train, y_train)
                             .map(torch::data::transforms::Stack<>());
    auto train_loader = torch::data::make_data_loader<torch::data::samplers::RandomSampler>(
        std::move(train_dataset), /*batch_size=*/32);

    auto test_dataset = torch::data::datasets::TensorDataset(x_test, y_test)
                            .map(torch::data::transforms::Stack<>());
    auto test_loader = torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(
        std::move(test_dataset), /*batch_size=*/32);

    // Get compiled neural network
    auto model = std::make_shared<TrafficNet>();
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(1e-3));

    // Fit model on training data
    std::cout << "Training model...\n";
    for (int epoch = 1; epoch <= EPOCHS; ++epoch) {
        model->train();
        double running_loss = 0.0;
        int batches = 0;

        for (auto& batch : *train_loader) {
            optimizer.zero_grad();
            torch::Tensor prediction = model->forward(batch.data);
            torch::Tensor loss = torch::nll_loss(prediction, batch.target);
            
            loss.backward();
            optimizer.step();
            
            running_loss += loss.item<double>();
            batches++;
        }
        std::cout << std::format("Epoch {}/{} - Loss: {:.4f}\n", epoch, EPOCHS, running_loss / batches);
    }

    // Evaluate neural network performance
    std::cout << "Evaluating model...\n";
    model->eval();
    int64_t correct = 0;
    
    // Disable gradient calculation for evaluation
    torch::NoGradGuard no_grad; 
    for (const auto& batch : *test_loader) {
        auto output = model->forward(batch.data);
        auto prediction = output.argmax(1);
        correct += prediction.eq(batch.target).sum().item<int64_t>();
    }
    
    double accuracy = static_cast<double>(correct) / test_count;
    std::cout << std::format("Test Accuracy: {:.4f}\n", accuracy);

    // Save model to file
    if (argc == 3) {
        std::string filename = argv[2];
        torch::save(model, filename);
        std::cout << std::format("Model saved to {}.\n", filename);
    }

    return EXIT_SUCCESS;
}
