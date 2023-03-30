from LSM9DS1_def import lib
import numpy as np
import matplotlib.pyplot as plt
import wiringpi


imu = lib.lsm9ds1_create()
lib.lsm9ds1_begin(imu)
if lib.lsm9ds1_begin(imu) == 0:
    print("Failed to communicate with LSM9DS1.")
    quit()
lib.lsm9ds1_calibrate(imu)

fig, ax = plt.subplots()

px = [0]
py=[0.0]
line, = ax.plot(px, py)


def animate(i):
    px.append(px[-1]+1)
    py.append(sin(i*1.0/360))
    line.set_xdata(px)
    line.set_ydata(py)  # update the data.
    return line,


ani = animation.FuncAnimation(
    fig, animate, interval=20, blit=True, save_count=50)

# To save the animation, use e.g.
#
# ani.save("movie.mp4")
#
# or
#
# writer = animation.FFMpegWriter(
#     fps=15, metadata=dict(artist='Me'), bitrate=1800)
# ani.save("movie.mp4", writer=writer)

plt.show()

while True:
    while lib.lsm9ds1_gyroAvailable(imu) == 0:
        pass
    lib.lsm9ds1_readGyro(imu)
    while lib.lsm9ds1_accelAvailable(imu) == 0:
        pass
    lib.lsm9ds1_readAccel(imu)
    while lib.lsm9ds1_magAvailable(imu) == 0:
        pass
    lib.lsm9ds1_readMag(imu)

    gx = lib.lsm9ds1_getGyroX(imu)
    gy = lib.lsm9ds1_getGyroY(imu)
    gz = lib.lsm9ds1_getGyroZ(imu)

    ax = lib.lsm9ds1_getAccelX(imu)
    ay = lib.lsm9ds1_getAccelY(imu)
    az = lib.lsm9ds1_getAccelZ(imu)

    mx = lib.lsm9ds1_getMagX(imu)
    my = lib.lsm9ds1_getMagY(imu)
    mz = lib.lsm9ds1_getMagZ(imu)

    cgx = lib.lsm9ds1_calcGyro(imu, gx)
    cgy = lib.lsm9ds1_calcGyro(imu, gy)
    cgz = lib.lsm9ds1_calcGyro(imu, gz)

    cax = lib.lsm9ds1_calcAccel(imu, ax)
    cay = lib.lsm9ds1_calcAccel(imu, ay)
    caz = lib.lsm9ds1_calcAccel(imu, az)

    cmx = lib.lsm9ds1_calcMag(imu, mx)
    cmy = lib.lsm9ds1_calcMag(imu, my)
    cmz = lib.lsm9ds1_calcMag(imu, mz)

    print("Gyro: %f, %f, %f [deg/s]" % (cgx, cgy, cgz))
    print("Accel: %f, %f, %f [Gs]" % (cax, cay, caz))
    print("Mag: %f, %f, %f [gauss]" % (cmx, cmy, cmz))

