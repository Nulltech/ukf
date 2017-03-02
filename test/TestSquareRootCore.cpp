#include <gtest/gtest.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>
#include "UKF/Types.h"
#include "UKF/Integrator.h"
#include "UKF/StateVector.h"
#include "UKF/MeasurementVector.h"
#include "UKF/Core.h"
#include "comparisons.h"

/*
Set up state vector class. The order of these is changed to prevent
linker collisions with the ones in TestCore.cpp.
*/
enum MyStateFields {
    Attitude,
    AngularVelocity,
    Position,
    Velocity
};

using MyStateVector = UKF::StateVector<
    UKF::Field<Position, UKF::Vector<3>>,
    UKF::Field<Velocity, UKF::Vector<3>>,
    UKF::Field<Attitude, UKF::Quaternion>,
    UKF::Field<AngularVelocity, UKF::Vector<3>>
>;

// specialization class is required
// extra namespace is needed due to a possible g++ bug
namespace UKF
{
    namespace Parameters
    {
        template<> struct AlphaSquared<MyStateVector> { static constexpr real_t value(){ return 1e-6; } };
    }
}

// MOD
//template <> constexpr real_t UKF::Parameters::AlphaSquared<MyStateVector> = 1e-6;

/*
State vector process model. One version takes body frame kinematic
acceleration and angular acceleration as inputs, the other doesn't (assumes
zero accelerations).
*/

// specialization class is required
// extra namespace is needed due to a possible g++ bug
namespace UKF
{
    template <> template <>
    MyStateVector MyStateVector::derivative<UKF::Vector<3>, UKF::Vector<3>>(
            const UKF::Vector<3>& acceleration, const UKF::Vector<3>& angular_acceleration) const {
        MyStateVector temp;
        
        /* Position derivative. */
        temp.set_field<Position>(get_field<Velocity>());

        /* Velocity derivative. */
        temp.set_field<Velocity>(get_field<Attitude>().conjugate() * acceleration);

        /* Attitude derivative. */
        UKF::Quaternion temp_q;
        temp_q.vec() = get_field<AngularVelocity>();
        temp_q.w() = 0;
        temp.set_field<Attitude>(temp_q);

        /* Angular velocity derivative. */
        temp.set_field<AngularVelocity>(angular_acceleration);

        return temp;
    }

    template <> template <>
    MyStateVector MyStateVector::derivative<>() const {
        return derivative(UKF::Vector<3>(0, 0, 0), UKF::Vector<3>(0, 0, 0));
    }
}

/*
Set up measurement vector class. The order of these is changed to prevent
linker collisions with the ones in TestCore.cpp.
*/
enum MyMeasurementFields {
    Accelerometer,
    Magnetometer,
    Gyroscope,
    GPS_Position,
    GPS_Velocity
};

using MyMeasurementVector = UKF::DynamicMeasurementVector<
    UKF::Field<GPS_Position, UKF::Vector<3>>,
    UKF::Field<GPS_Velocity, UKF::Vector<3>>,
    UKF::Field<Accelerometer, UKF::Vector<3>>,
    UKF::Field<Magnetometer, UKF::FieldVector>,
    UKF::Field<Gyroscope, UKF::Vector<3>>
>;

using MyCore = UKF::SquareRootCore<
    MyStateVector,
    MyMeasurementVector,
    UKF::IntegratorRK4
>;

/*
Define measurement model to be used in tests. NOTE: These are just for
testing, don't expect them to make any physical sense whatsoever.
*/

// specialization class is required
// extra namespace is needed due to a possible g++ bug
namespace UKF
{
    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, GPS_Position>(const MyStateVector& state) {
        return state.get_field<Position>();
    }

    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, GPS_Velocity>(const MyStateVector& state) {
        return state.get_field<Velocity>();
    }

    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, Accelerometer>(const MyStateVector& state) {
        return state.get_field<Attitude>() * UKF::Vector<3>(0, 0, -9.8);
    }

    template <> template <>
    UKF::FieldVector MyMeasurementVector::expected_measurement
    <MyStateVector, Magnetometer>(const MyStateVector& state) {
        return state.get_field<Attitude>() * UKF::FieldVector(1, 0, 0);
    }

    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, Gyroscope>(const MyStateVector& state) {
        return state.get_field<AngularVelocity>();
    }
}

/*
These versions of the predicted measurement functions take kinematic
acceleration and angular acceleration as inputs. Note that in reality, the
inputs would probably be a control vector and the accelerations would be
calculated using the state vector and a dynamics model.
*/

// specialization class is required
// extra namespace is needed due to a possible g++ bug
namespace UKF
{
    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, GPS_Position>(const MyStateVector& state,
            const UKF::Vector<3>& acceleration, const UKF::Vector<3>& angular_acceleration) {
        return state.get_field<Position>();
    }

    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, GPS_Velocity>(const MyStateVector& state,
            const UKF::Vector<3>& acceleration, const UKF::Vector<3>& angular_acceleration) {
        return state.get_field<Velocity>();
    }

    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, Accelerometer, UKF::Vector<3>>(const MyStateVector& state,
            const UKF::Vector<3>& acceleration, const UKF::Vector<3>& angular_acceleration) {
        return state.get_field<Attitude>() * UKF::Vector<3>(0, 0, -9.8) + acceleration;
    }

    template <> template <>
    UKF::FieldVector MyMeasurementVector::expected_measurement
    <MyStateVector, Magnetometer, UKF::Vector<3>>(const MyStateVector& state,
            const UKF::Vector<3>& acceleration, const UKF::Vector<3>& angular_acceleration) {
        return state.get_field<Attitude>() * UKF::FieldVector(1, 0, 0);
    }

    template <> template <>
    UKF::Vector<3> MyMeasurementVector::expected_measurement
    <MyStateVector, Gyroscope, UKF::Vector<3>>(const MyStateVector& state,
            const UKF::Vector<3>& acceleration, const UKF::Vector<3>& angular_acceleration) {
        return state.get_field<AngularVelocity>();
    }
}

/* Set the measurement covariance vector. */
template <>
MyMeasurementVector::CovarianceVector MyMeasurementVector::measurement_root_covariance =
    MyMeasurementVector::CovarianceVector();

/*
Initialise covariances as square roots to be comparable with TestCore.cpp.
*/
MyCore create_initialised_sr_test_filter() {
    MyMeasurementVector::measurement_root_covariance << 
        10, 10, 10, 1, 1, 1, 5e-1, 5e-1, 5e-1, 5e-1, 5e-1, 5e-1, 0.05, 0.05, 0.05;
    MyMeasurementVector::measurement_root_covariance = MyMeasurementVector::measurement_root_covariance.array().sqrt();
    MyCore test_filter;
    test_filter.state.set_field<Position>(UKF::Vector<3>(0, 0, 0));
    test_filter.state.set_field<Velocity>(UKF::Vector<3>(0, 0, 0));
    test_filter.state.set_field<Attitude>(UKF::Quaternion(1, 0, 0, 0));
    test_filter.state.set_field<AngularVelocity>(UKF::Vector<3>(0, 0, 0));
    test_filter.root_covariance = MyStateVector::CovarianceMatrix::Zero();
    test_filter.root_covariance.diagonal() <<
        10000, 10000, 10000, 100, 100, 100, 1, 1, 5, 10, 10, 10;
    test_filter.root_covariance = test_filter.root_covariance.llt().matrixU();

    real_t a, b;
    real_t dt = 0.01;
    a = std::sqrt(0.1*dt*dt);
    b = std::sqrt(0.1*dt);
    test_filter.process_noise_root_covariance << a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, a, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, b, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, b, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, b, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, a, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, a, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, a, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, b, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, b, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, b;
    test_filter.process_noise_root_covariance = test_filter.process_noise_root_covariance.llt().matrixU();

    return test_filter;
}

TEST(SquareRootCoreTest, Initialisation) {
    MyCore test_filter = create_initialised_sr_test_filter();
}

/*
All these tests check that the estimated state matches the 'actual' state to
within 2-sigma.
*/
TEST(SquareRootCoreTest, APrioriStep) {
    MyCore test_filter = create_initialised_sr_test_filter();

    test_filter.a_priori_step(0.01);

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}

TEST(SquareRootCoreTest, APrioriStepWithInputs) {
    MyCore test_filter = create_initialised_sr_test_filter();

    test_filter.a_priori_step(0.01, UKF::Vector<3>(0, 0, -5), UKF::Vector<3>(1, 0, 0));

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}

TEST(SquareRootCoreTest, InnovationStep) {
    MyCore test_filter = create_initialised_sr_test_filter();
    MyMeasurementVector m;

    m.set_field<GPS_Position>(UKF::Vector<3>(100, 10, -50));
    m.set_field<GPS_Velocity>(UKF::Vector<3>(20, 0, 0));
    m.set_field<Accelerometer>(UKF::Vector<3>(0, 0, -9.8));
    m.set_field<Magnetometer>(UKF::FieldVector(0, -1, 0));
    m.set_field<Gyroscope>(UKF::Vector<3>(0.5, 0, 0));

    test_filter.a_priori_step(0.01);
    test_filter.innovation_step(m);

    /*
    With the field vector, we expect the determinant to be approximately zero,
    so allow for it to be slightly negative due to numerical precision.
    */
    EXPECT_GE(test_filter.innovation_root_covariance.determinant(), -std::numeric_limits<real_t>::epsilon());

    test_filter.a_posteriori_step();

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}

TEST(SquareRootCoreTest, InnovationStepPartialMeasurement) {
    MyCore test_filter = create_initialised_sr_test_filter();
    MyMeasurementVector m;

    m.set_field<Accelerometer>(UKF::Vector<3>(0, 0, -9.8));
    m.set_field<Magnetometer>(UKF::FieldVector(0, -1, 0));
    m.set_field<Gyroscope>(UKF::Vector<3>(0.5, 0, 0));

    test_filter.a_priori_step(0.01);
    test_filter.innovation_step(m);

    EXPECT_GE(test_filter.innovation_root_covariance.determinant(), -std::numeric_limits<real_t>::epsilon());

    test_filter.a_posteriori_step();

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}

TEST(SquareRootCoreTest, InnovationStepWithInputs) {
    MyCore test_filter = create_initialised_sr_test_filter();
    MyMeasurementVector m;

    m.set_field<GPS_Position>(UKF::Vector<3>(100, 10, -50));
    m.set_field<GPS_Velocity>(UKF::Vector<3>(20, 0, 0));
    m.set_field<Accelerometer>(UKF::Vector<3>(0, 0, -15));
    m.set_field<Magnetometer>(UKF::FieldVector(0, -1, 0));
    m.set_field<Gyroscope>(UKF::Vector<3>(0.5, 0, 0));

    test_filter.a_priori_step(0.01, UKF::Vector<3>(0, 0, -5), UKF::Vector<3>(1, 0, 0));
    test_filter.innovation_step(m, UKF::Vector<3>(0, 0, -5), UKF::Vector<3>(1, 0, 0));

    EXPECT_GE(test_filter.innovation_root_covariance.determinant(), -std::numeric_limits<real_t>::epsilon());

    test_filter.a_posteriori_step();

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}

TEST(SquareRootCoreTest, InnovationStepPartialMeasurementWithInputs) {
    MyCore test_filter = create_initialised_sr_test_filter();
    MyMeasurementVector m;

    m.set_field<Accelerometer>(UKF::Vector<3>(0, 0, -15));
    m.set_field<Magnetometer>(UKF::FieldVector(0, -1, 0));
    m.set_field<Gyroscope>(UKF::Vector<3>(0.5, 0, 0));

    test_filter.a_priori_step(0.01, UKF::Vector<3>(0, 0, -5), UKF::Vector<3>(1, 0, 0));
    test_filter.innovation_step(m, UKF::Vector<3>(0, 0, -5), UKF::Vector<3>(1, 0, 0));

    EXPECT_GE(test_filter.innovation_root_covariance.determinant(), -std::numeric_limits<real_t>::epsilon());

    test_filter.a_posteriori_step();

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}

TEST(SquareRootCoreTest, APosterioriStep) {
    MyCore test_filter = create_initialised_sr_test_filter();
    MyMeasurementVector m;

    m.set_field<GPS_Position>(UKF::Vector<3>(100, 10, -50));
    m.set_field<GPS_Velocity>(UKF::Vector<3>(20, 0, 0));
    m.set_field<Accelerometer>(UKF::Vector<3>(0, 0, -9.8));
    m.set_field<Magnetometer>(UKF::FieldVector(0, -1, 0));
    m.set_field<Gyroscope>(UKF::Vector<3>(0.5, 0, 0));

    test_filter.a_priori_step(0.01);
    test_filter.innovation_step(m);
    test_filter.a_posteriori_step();

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}

TEST(SquareRootCoreTest, FullStep) {
    MyCore test_filter = create_initialised_sr_test_filter();
    MyMeasurementVector m;

    m.set_field<GPS_Position>(UKF::Vector<3>(100, 10, -50));
    m.set_field<GPS_Velocity>(UKF::Vector<3>(20, 0, 0));
    m.set_field<Accelerometer>(UKF::Vector<3>(0, 0, -14.8));
    m.set_field<Magnetometer>(UKF::FieldVector(0, -1, 0));
    m.set_field<Gyroscope>(UKF::Vector<3>(0.5, 0, 0));

    test_filter.step(0.01, m, UKF::Vector<3>(0, 0, -5), UKF::Vector<3>(1, 0, 0));

    EXPECT_GT(test_filter.root_covariance.determinant(), std::numeric_limits<real_t>::epsilon());
    EXPECT_LT((UKF::Vector<3>(100, 10, -50) - test_filter.state.get_field<Position>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(0).norm()*2);
    EXPECT_LT((UKF::Vector<3>(20, 0, 0) - test_filter.state.get_field<Velocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(3).norm()*2);
    EXPECT_LT(2*std::acos(std::abs(UKF::Quaternion(0.7071, 0, 0, -0.7071).dot(test_filter.state.get_field<Attitude>()))),
        test_filter.root_covariance.diagonal().segment<3>(6).norm()*2);
    EXPECT_LT((UKF::Vector<3>(0.5, 0, 0) - test_filter.state.get_field<AngularVelocity>()).norm(),
        test_filter.root_covariance.diagonal().segment<3>(9).norm()*2);
}
