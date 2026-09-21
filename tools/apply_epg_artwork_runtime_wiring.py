#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected anchor exactly once, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


context = Path("core/daemon/include/BackendRuntimeContext.h")
replace_once(context, '#include "EpgArtworkRepository.h"\n', '')
replace_once(context, '    std::unique_ptr<EpgArtworkRepository> epgArtworkRepository;\n', '')

runtime_header = Path("core/daemon/include/DaemonRuntime.h")
replace_once(
    runtime_header,
    '#include "EpgCacheController.h"\n',
    '#include "EpgArtworkPublicJsonSerializer.h"\n#include "EpgArtworkRepository.h"\n#include "EpgCacheController.h"\n',
)
replace_once(
    runtime_header,
    '    std::unique_ptr<EpgEventRepository> epgEventRepository_;\n',
    '    std::unique_ptr<EpgEventRepository> epgEventRepository_;\n'
    '    std::unique_ptr<EpgArtworkRepository> epgArtworkRepository_;\n'
    '    std::unique_ptr<EpgArtworkPublicJsonSerializer> epgArtworkPublicJsonSerializer_;\n',
)

runtime_cpp = Path("core/daemon/src/DaemonRuntime.cpp")
old_repository_block = '''        context->epgArtworkRepository =
            std::make_unique<EpgArtworkRepository>(database_);

        if (!context->epgArtworkRepository->ensureSchema()) {
            std::cerr
                << "failed to initialize EPG artwork repository schema for backend "
                << context->backendId
                << std::endl;
            context->epgArtworkRepository.reset();
        }
        else {
            vdrsuite::agent::SuiteBridgeSvdrpTransportConfig artworkTransportConfig;
            artworkTransportConfig.host = suiteBridgeConfig.host;
            artworkTransportConfig.port = suiteBridgeConfig.port;
            artworkTransportConfig.connectTimeout =
                std::chrono::milliseconds(suiteBridgeConfig.connectTimeoutMs);
            artworkTransportConfig.ioTimeout =
                std::chrono::milliseconds(suiteBridgeConfig.ioTimeoutMs);
            artworkTransportConfig.operationTimeout =
                std::chrono::milliseconds(suiteBridgeConfig.operationTimeoutMs);

            context->epgArtworkTransport =
                std::make_unique<vdrsuite::agent::SuiteBridgeSvdrpTransport>(
                    std::move(artworkTransportConfig));
            context->epgArtworkResolver =
                std::make_unique<SuiteBridgeEpgArtworkResolver>(
                    *context->epgArtworkTransport);
            context->epgArtworkEnrichmentService =
                std::make_unique<EpgArtworkEnrichmentService>(
                    *context->epgArtworkRepository,
                    *context->epgArtworkResolver);
        }
'''
new_repository_block = '''        if (epgArtworkRepository_) {
            vdrsuite::agent::SuiteBridgeSvdrpTransportConfig artworkTransportConfig;
            artworkTransportConfig.host = suiteBridgeConfig.host;
            artworkTransportConfig.port = suiteBridgeConfig.port;
            artworkTransportConfig.connectTimeout =
                std::chrono::milliseconds(suiteBridgeConfig.connectTimeoutMs);
            artworkTransportConfig.ioTimeout =
                std::chrono::milliseconds(suiteBridgeConfig.ioTimeoutMs);
            artworkTransportConfig.operationTimeout =
                std::chrono::milliseconds(suiteBridgeConfig.operationTimeoutMs);

            context->epgArtworkTransport =
                std::make_unique<vdrsuite::agent::SuiteBridgeSvdrpTransport>(
                    std::move(artworkTransportConfig));
            context->epgArtworkResolver =
                std::make_unique<SuiteBridgeEpgArtworkResolver>(
                    *context->epgArtworkTransport);
            context->epgArtworkEnrichmentService =
                std::make_unique<EpgArtworkEnrichmentService>(
                    *epgArtworkRepository_,
                    *context->epgArtworkResolver);
        }
'''
replace_once(runtime_cpp, old_repository_block, new_repository_block)

replace_once(
    runtime_cpp,
    '''    if (!epgEventRepository_->ensureSchema()) {
        std::cerr << "failed to initialize EPG cache repository schema" << std::endl;
        return false;
    }

''',
    '''    if (!epgEventRepository_->ensureSchema()) {
        std::cerr << "failed to initialize EPG cache repository schema" << std::endl;
        return false;
    }

    epgArtworkRepository_ = std::make_unique<EpgArtworkRepository>(database_);

    if (!epgArtworkRepository_->ensureSchema()) {
        std::cerr << "failed to initialize EPG artwork repository schema" << std::endl;
        return false;
    }

    epgArtworkPublicJsonSerializer_ =
        std::make_unique<EpgArtworkPublicJsonSerializer>();

''',
)

replace_once(
    runtime_cpp,
    '''    epgCacheController_ = std::make_unique<EpgCacheController>(
        *epgCacheServiceRegistry_);
''',
    '''    epgCacheController_ = std::make_unique<EpgCacheController>(
        *epgCacheServiceRegistry_,
        *epgArtworkRepository_,
        *epgArtworkPublicJsonSerializer_);
''',
)

print("EPG artwork runtime wiring applied")
