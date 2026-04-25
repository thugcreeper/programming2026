import cv2
import cv2.aruco as aruco
import matplotlib.pyplot as plt

# 作業3bonus 產生自己的marker
arucoDict = aruco.getPredefinedDictionary(aruco.DICT_7X7_50)
plt.figure(figsize=(10, 10))
for i in range(6):
    plt.title(i)
    marker_image = aruco.generateImageMarker(arucoDict, i, 200)
    plt.subplot(3, 2, i + 1)
    plt.imshow(marker_image, cmap="gray")
    plt.axis(False)
plt.show()
