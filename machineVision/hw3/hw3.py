"""1. Calibrate the camera by the ChArUco board in the video file "CharUco_board.mp4".
    Show the camera matrix and distortion coefficients.


2. Detect the six ArUco markers in the video file "arUco_marker.mp4".
    Show six different videos on those markers for augmented reality.
3. (bonus) You can create your own ArUco markers for this homework.

4. Write a report for this homework.
"""

import cv2
import cv2.aruco as aruco
import numpy as np


def hw3(input_video_path, output_video_path, flip_or_not=False):
    # read ChAruco board video
    cap = cv2.VideoCapture("ChArUco_board.mp4")
    # 原始畫面有點大，為了有利於顯示這份講義所以縮小。
    totalFrame = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    frameWidth = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)) // 2
    frameHeight = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)) // 2
    # 紀錄fps避免輸出影片的速度很奇怪
    fps = cap.get(cv2.CAP_PROP_FPS)
    arucoParams = aruco.DetectorParameters()
    arucoParams.cornerRefinementMethod = aruco.CORNER_REFINE_SUBPIX
    arucoDict = aruco.getPredefinedDictionary(aruco.DICT_6X6_250)

    # 必須描述ChArUco board的尺寸規格
    gridX = 5  # 水平方向5格
    gridY = 7  # 垂直方向7格
    squareSize = 4  # 每格為4cmX4cm

    # ArUco marker為2cmX2cm
    charucoBoard = aruco.CharucoBoard(
        (gridX, gridY), squareSize, squareSize / 2, arucoDict
    )

    cameraMatrixInit = np.array(
        [
            [1000.0, 0.0, frameWidth / 2.0],
            [0.0, 1000.0, frameHeight / 2.0],
            [0.0, 0.0, 1.0],
        ]
    )
    distCoeffsInit = np.zeros((5, 1))

    charucoParams = aruco.CharucoParameters()
    charucoParams.tryRefineMarkers = True
    charucoParams.cameraMatrix = cameraMatrixInit
    charucoParams.distCoeffs = distCoeffsInit
    charucoDetector = aruco.CharucoDetector(charucoBoard, charucoParams, arucoParams)

    print(
        "height {}, width {} ,fps {}".format(
            cap.get(cv2.CAP_PROP_FRAME_HEIGHT), cap.get(cv2.CAP_PROP_FRAME_WIDTH), fps
        )
    )
    refinedStrategy = True
    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 100, 0.00001)
    frameId = 0
    objPointsList = []
    imgPointsList = []

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame = cv2.resize(frame, (frameWidth, frameHeight))
        corners, ids, _, _ = charucoDetector.detectBoard(frame)
        if corners is not None and corners.shape[0] > 0:
            aruco.drawDetectedCornersCharuco(frame, corners, ids)
            # objPoints是3D點(世界坐標系)，imgPoints是2D點
            objPoints, imgPoints = charucoBoard.matchImagePoints(corners, ids)

            if frameId % 100 == 50 and objPoints.shape[0] >= 4:
                objPointsList.append(objPoints)
                imgPointsList.append(imgPoints)
        if ids is not None:
            # print(f"ids: {ids.flatten()}")
            pass
        # cv2.imshow("Analysis of a CharUco board for camera calibration", frame)

        frameId += 1
    # 相機校正
    ret, charuco_cameraMatrix, charuco_distCoeffs, aruco_rvects, aruco_tvects = (
        cv2.calibrateCamera(
            objPointsList,
            imgPointsList,
            (frameWidth, frameHeight),
            cameraMatrixInit,
            distCoeffsInit,
        )
    )
    print(charuco_cameraMatrix)
    print(charuco_distCoeffs)
    cap.release()
    cv2.destroyAllWindows()
    # 對應到6個marker的影片

    # 建立儲存物件
    fourcc = cv2.VideoWriter_fourcc(*"XVID")  # fourcc是
    videoWriter = cv2.VideoWriter(
        output_video_path, fourcc, fps, (frameWidth, frameHeight)
    )
    if not videoWriter.isOpened():
        print("Error: Cannot open video writer.")
    # 加入要疊加的影片
    overlay_videos = []
    for i in range(1, 7):
        video_path = f"videos/video{i}.mp4"
        v_cap = cv2.VideoCapture(video_path)
        if not v_cap.isOpened():
            print(f"Warning: Cannot open {video_path}")
        overlay_videos.append(v_cap)
        print(f"Add video {i} successfully.")

    # 開啟6個marker的影片
    # aruco_cap = cv2.VideoCapture("arUco_marker.mp4")
    aruco_cap = cv2.VideoCapture(input_video_path)
    arucoDict = aruco.getPredefinedDictionary(aruco.DICT_7X7_50)
    arucoDetector = cv2.aruco.ArucoDetector(arucoDict, arucoParams)
    # 定義marker的3D座標
    s = 2.0  # marker邊長
    marker_obj_points = np.array(
        [
            [-s / 2, s / 2, 0],  # 左上
            [s / 2, s / 2, 0],  # 右上
            [s / 2, -s / 2, 0],  # 右下
            [-s / 2, -s / 2, 0],  # 左下
        ],
        dtype=np.float32,
    )
    while True:
        ret, frame = aruco_cap.read()
        if not ret:
            break
        frame = cv2.resize(frame, (frameWidth, frameHeight))
        corners, ids, rejected = arucoDetector.detectMarkers(frame)
        # print(len(corners))
        if ids is not None:
            for i in range(len(ids)):
                marker_id = ids[i][0]
                video_idx = marker_id % len(overlay_videos)
                v_cap = overlay_videos[video_idx]

                v_ret, v_frame = v_cap.read()
                if not v_ret:  # 影片播完了就重頭開始
                    v_cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                    v_ret, v_frame = v_cap.read()
                # solvePnP是用來從3D物體點和對應的2D圖像點計算相機位置和方向的函數。
                ret, rvect, tvect = cv2.solvePnP(
                    marker_obj_points,
                    corners[i][0],
                    charuco_cameraMatrix,
                    charuco_distCoeffs,
                )
                # 將marker的四個角點透過project points投影到2D圖像上
                proj_pt_with_dist, _ = cv2.projectPoints(
                    marker_obj_points,
                    rvect,
                    tvect,
                    charuco_cameraMatrix,
                    charuco_distCoeffs,
                )

                if v_ret:
                    if flip_or_not:
                        v_frame = cv2.flip(v_frame, 0)
                    # flip選0表示翻轉x軸
                    # v_frame_flipped = cv2.flip(v_frame, 0)
                    # 取得影片的四個角 (左上, 右上, 右下, 左下)
                    v_h, v_w = v_frame.shape[:2]
                    src_pts = np.float32([[0, 0], [v_w, 0], [v_w, v_h], [0, v_h]])
                    # print("src_pts shape:", src_pts.shape) output:src_pts shape: (4, 2)
                    # 計算矩陣並變換
                    # reshape(-1,2)中，-1表示自動計算行數，2表示每行有兩個元素（x和y座標）
                    matrix = cv2.getPerspectiveTransform(
                        src_pts, proj_pt_with_dist.reshape(-1, 2).astype(np.float32)
                    )
                    # 用透視變換將影片疊加到對應的marker位置
                    warped_v = cv2.warpPerspective(
                        v_frame, matrix, (frameWidth, frameHeight)
                    )

                    # 建立遮罩將影片蓋上去
                    # mask shape: (540, 960)
                    mask = np.zeros((frameHeight, frameWidth), dtype=np.uint8)
                    # fillConvexPoly負責將dst_pts所形成的多邊形區域填充為255（白色），
                    cv2.fillConvexPoly(
                        mask, proj_pt_with_dist.reshape(-1, 2).astype(np.int32), 255
                    )

                    # framebg看起來像是原始畫面扣掉影片要疊加的區域
                    # bitwise and只有當兩張圖片的對應像素都是255時，結果才會是255，這樣就可以得到
                    # 原始畫面中除了影片要疊加的區域以外的部分。
                    frame_bg = cv2.bitwise_and(frame, frame, mask=cv2.bitwise_not(mask))
                    # cv2.add將圖片疊加
                    frame = cv2.add(frame_bg, warped_v)
                    frame = frame.astype(np.uint8)
        videoWriter.write(frame)
        cv2.imshow("AR Result", frame)
        if cv2.waitKey(20) & 0xFF == 27:  # 按下 ESC 退出
            break
    # 釋放資源
    videoWriter.release()
    aruco_cap.release()
    for v in overlay_videos:
        v.release()
    cv2.destroyAllWindows()
    print(f"影片已儲存{videoWriter.filename}")


# 作業範例
# hw3("ArUco_marker.mp4", "output.avi", True)
# 我自己的marker
hw3("myMarker.mp4", "myMarker_output.avi", False)
