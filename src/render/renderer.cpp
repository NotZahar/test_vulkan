#include "renderer.hpp"

#include <format>
#include <cassert>

#include "../logger.hpp"
#include "../utility/messages.hpp"

namespace tv {
    Renderer::Renderer() noexcept
        : _mainWindow{ nullptr },
          _vInstance{ nullptr }
    {
        glfwInit();
        makeVInstance();
        initWindow();
    }

    Renderer::~Renderer() {
        glfwDestroyWindow(_mainWindow);
        _vInstance.destroy();
        glfwTerminate();
    }

    void Renderer::initWindow() noexcept {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        _mainWindow = glfwCreateWindow(
            _windowWidth,
            _windowHeight,
            constants::messages::WINDOW_TITLE.c_str(),
            nullptr,
            nullptr
        );
    }

    Renderer& Renderer::instance() noexcept {
        static Renderer instance;
        return instance;
    }

    void Renderer::processEvents() noexcept {
        while (!glfwWindowShouldClose(_mainWindow))
            glfwPollEvents();
    }

    void Renderer::printAdditionalInfo(const uint32_t vulkanVersion, const std::vector<const char*>& glfwExtensions) noexcept {
        using namespace constants;

        const std::string vulkanVersionMessage = std::format(
            "{}: {}\n"
            "{}: {}\n"
            "{}: {}\n"
            "{}: {}\n",
            messages::VULKAN_CONFIG_VARIANT, VK_API_VERSION_VARIANT(vulkanVersion),
            messages::VULKAN_CONFIG_MAJOR, VK_API_VERSION_MAJOR(vulkanVersion),
            messages::VULKAN_CONFIG_MINOR, VK_API_VERSION_MINOR(vulkanVersion),
            messages::VULKAN_CONFIG_PATCH, VK_API_VERSION_PATCH(vulkanVersion));

        std::string glfwExtensionsMessage = std::format("{}:\n", messages::GLFW_EXTENSIONS);
        for (auto name : glfwExtensions)
            glfwExtensionsMessage += std::format("    {}\n", name);

        auto& logger = Logger::instance();
        logger.log(vulkanVersionMessage);
        logger.log(glfwExtensionsMessage);
    }

    vk::Instance Renderer::makeVInstance() noexcept {
        using namespace constants;

        assert(glfwVulkanSupported());

        uint32_t vulkanVersion{ 0 };
        vkEnumerateInstanceVersion(&vulkanVersion);

        uint32_t glfwExtensionCount = 0;
        auto rawGlfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> glfwExtensions(rawGlfwExtensions, rawGlfwExtensions + glfwExtensionCount);

        vk::ApplicationInfo appInfo{
            messages::APP_NAME.c_str(),
            vulkanVersion,
            nullptr,
            vulkanVersion,
            vulkanVersion
        };

        vk::InstanceCreateInfo createInfo{
            vk::InstanceCreateFlags(),
            &appInfo,
            0,
            nullptr,
            static_cast<uint32_t>(glfwExtensions.size()),
            glfwExtensions.data()
        };

        printAdditionalInfo(vulkanVersion, glfwExtensions);

        try {
            return vk::createInstance(createInfo);
        } catch (const vk::SystemError& err) {
            Logger::instance().err(std::format("{}: {}",
                messages::VULKAN_INSTANCE_CREATION_FAILED,
                err.what()));
            return nullptr;
        }
    }
}

