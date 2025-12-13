#pragma once

#include <QObject>
#include <QRhi>
#include <QRhiTexture>
#include <QString>
#include <memory>
#include <unordered_map>

namespace NeuralStudio {
    namespace Rendering {

        /**
 * @brief STMapLoader - Loads and manages STMap UV remap textures
 * 
 * STMaps are TIFF or PNG images where:
 * - Red channel = U coordinate (0-1)
 * - Green channel = V coordinate (0-1)
 * 
 * Used for fisheye lens distortion correction.
 * Each lens/camera has a unique STMap calibration file.
 */
        class STMapLoader : public QObject
        {
            Q_OBJECT

              public:
            explicit STMapLoader(QRhi *rhi, QObject *parent = nullptr);
            ~STMapLoader() override;

            /**
     * @brief Load an STMap from file
     * @param path Path to TIFF or PNG file
     * @param id Unique identifier for this STMap
     * @return true if loaded successfully
     */
            bool loadSTMap(const QString &path, const QString &id);

            /**
     * @brief Get loaded STMap texture
     * @param id STMap identifier
     * @return Texture pointer, or nullptr if not found
     */
            QRhiTexture *getSTMap(const QString &id);

            /**
     * @brief Unload an STMap
     */
            void unloadSTMap(const QString &id);

            /**
     * @brief Unload all STMaps
     */
            void unloadAll();

            /**
     * @brief Check if STMap is loaded
     */
            bool isLoaded(const QString &id) const;

            /**
     * @brief Get loaded STMap count
     */
            int getLoadedCount() const
            {
                return m_stmaps.size();
            }

              signals:
            void stmapLoaded(const QString &id);
            void stmapUnloaded(const QString &id);
            void loadError(const QString &path, const QString &error);

              private:
            struct STMapData {
                QString id;
                QString path;
                std::unique_ptr<QRhiTexture> texture;
                uint32_t width;
                uint32_t height;
            };

            QRhi *m_rhi;
            std::unordered_map<QString, STMapData> m_stmaps;
        };

    }  // namespace Rendering
}  // namespace NeuralStudio
