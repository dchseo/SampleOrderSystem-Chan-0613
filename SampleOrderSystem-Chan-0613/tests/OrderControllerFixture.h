#pragma once
#include <filesystem>
#include <string>

#include "../Controller/MonitoringController.h"
#include "../Controller/OrderController.h"
#include "../Controller/SampleController.h"
#include "../Model/ProductionLine.h"
#include "../Model/Repository/JsonOrderRepository.h"
#include "../Model/Repository/JsonProductionLineRepository.h"
#include "../Model/Repository/JsonSampleRepository.h"

// OrderController Unit Test / 적대적 테스트가 공통으로 쓰는 픽스처.
// 매 테스트마다 이름이 겹치지 않는 임시 JSON 파일로 독립된 Repository를 구성한다.
namespace TestSupport
{
    inline std::string TempFilePath(const std::string& fileName)
    {
        return (std::filesystem::temp_directory_path() / ("SampleOrderSystem_Controller_" + fileName)).string();
    }

    // 이전 테스트 실행이 남긴 파일이 있으면 지우고, 항상 빈 상태에서 시작하게 한다.
    inline std::string FreshTempFilePath(const std::string& fileName)
    {
        const std::string path = TempFilePath(fileName);
        std::filesystem::remove(path);
        return path;
    }

    struct OrderControllerFixture
    {
        explicit OrderControllerFixture(const std::string& testName)
            : sampleRepo(FreshTempFilePath(testName + "_samples.json"))
            , orderRepo(FreshTempFilePath(testName + "_orders.json"))
            , productionLineRepo(FreshTempFilePath(testName + "_queue.json"))
            , sampleController(sampleRepo)
            , monitoringController(orderRepo, sampleRepo)
            , orderController(orderRepo, sampleRepo, productionLine, productionLineRepo)
        {
        }

        Model::JsonSampleRepository sampleRepo;
        Model::JsonOrderRepository orderRepo;
        Model::ProductionLine productionLine;
        Model::JsonProductionLineRepository productionLineRepo;
        Controller::SampleController sampleController;
        Controller::MonitoringController monitoringController;
        Controller::OrderController orderController;
    };
}
