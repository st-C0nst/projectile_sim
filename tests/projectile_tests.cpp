#include "projectile_sim/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <gtest/gtest.h>
#include <limits>
#include <random>
#include <string>
#include <type_traits>

namespace {
// Calculate each component in double precision without calling simulation
// helpers. Compare against the representable input values, including the actual
// dt/gravity.
template <pdef::ProjectileVector Vec>
void expect_ballistic(const pdef::BaseProjectile<Vec> &actual,
                      const pdef::BaseProjectile<Vec> &initial,
                      const glm::dvec3 &acceleration, double time,
                      double absolute = 1e-5, double relative = 1e-6,
                      double lifetime_absolute = -1) {
  for (int axis = 0; axis < 3; ++axis) {
    SCOPED_TRACE(axis);
    const double position = double(initial.position[axis]) +
                            double(initial.velocity[axis]) * time +
                            0.5 * acceleration[axis] * time * time;
    const double velocity =
        double(initial.velocity[axis]) + acceleration[axis] * time;
    EXPECT_NEAR(double(actual.position[axis]), position,
                absolute + relative * std::abs(position));
    EXPECT_NEAR(double(actual.velocity[axis]), velocity,
                absolute + relative * std::abs(velocity));
  }
  const double lifetime = double(initial.lifetime) - time;
  EXPECT_NEAR(double(actual.lifetime), lifetime,
              (lifetime_absolute < 0 ? absolute : lifetime_absolute) +
                  relative * std::abs(lifetime));
  EXPECT_EQ(actual.type, initial.type);
}

template <pdef::ProjectileVector Vec>
void expect_same(const pdef::BaseProjectile<Vec> &actual,
                 const pdef::BaseProjectile<Vec> &expected) {
  for (int axis = 0; axis < 3; ++axis) {
    EXPECT_EQ(actual.position[axis], expected.position[axis]);
    EXPECT_EQ(actual.velocity[axis], expected.velocity[axis]);
  }
  EXPECT_EQ(actual.lifetime, expected.lifetime);
  EXPECT_EQ(actual.type, expected.type);
}

template <typename Vec> class ProjectileTest : public testing::Test {
public:
  using Scalar = typename Vec::value_type;
  using Projectile = pdef::BaseProjectile<Vec>;
  Projectile initial{{1, 2, 3}, {4, 5, 6}, 10, 42};
};
using Precisions = testing::Types<glm::vec3, glm::dvec3>;
TYPED_TEST_SUITE(ProjectileTest, Precisions);

TYPED_TEST(ProjectileTest, DefaultStateIsZero) {
  expect_same(typename TestFixture::Projectile{},
              typename TestFixture::Projectile{{0, 0, 0}, {0, 0, 0}, 0, 0});
}

TYPED_TEST(ProjectileTest, ConversionPreservesAllFields) {
  using Other = std::conditional_t<std::is_same_v<TypeParam, glm::vec3>,
                                   glm::dvec3, glm::vec3>;
  using OtherScalar = typename Other::value_type;
  const typename TestFixture::Projectile source{
      TypeParam{0.1, -123.456, 789.123}, TypeParam{-0.2, 456.789, -987.654},
      typename TestFixture::Scalar(1.23456789),
      std::numeric_limits<pdef::ProjectileId>::max()};
  const auto converted = pdef::projectile_cast<Other>(source);
  for (int axis = 0; axis < 3; ++axis) {
    EXPECT_EQ(converted.position[axis],
              static_cast<OtherScalar>(source.position[axis]));
    EXPECT_EQ(converted.velocity[axis],
              static_cast<OtherScalar>(source.velocity[axis]));
  }
  EXPECT_EQ(converted.lifetime, static_cast<OtherScalar>(source.lifetime));
  EXPECT_EQ(converted.type, source.type);
}

TYPED_TEST(ProjectileTest, ZeroTimeLeavesBallisticStateUnchanged) {
  expect_same(pdef::update_balistic(this->initial, TypeParam{0, -10, 0}, 0),
              this->initial);
}

TYPED_TEST(ProjectileTest, ZeroAccelerationProducesLinearMotion) {
  const auto result = pdef::update_balistic(this->initial, TypeParam{0}, 2);
  expect_same(result,
              typename TestFixture::Projectile{{9, 12, 15}, {4, 5, 6}, 8, 42});
}

TYPED_TEST(ProjectileTest, KnownBallisticMotion) {
  // At t=2: p=(1,2,3)+(4,5,6)*2+(0,-10,0)*2; v=(4,-15,6).
  const TypeParam acceleration{0, -10, 0};
  const auto position = pdef::make_position(this->initial, acceleration, 2);
  const auto velocity = pdef::make_velocity(this->initial, acceleration, 2);
  EXPECT_EQ(position, (TypeParam{9, -8, 15}));
  EXPECT_EQ(velocity, (TypeParam{4, -15, 6}));
  expect_same(
      pdef::update_balistic(this->initial, acceleration, 2),
      typename TestFixture::Projectile{{9, -8, 15}, {4, -15, 6}, 8, 42});
  expect_same(this->initial,
              typename TestFixture::Projectile{{1, 2, 3}, {4, 5, 6}, 10, 42});
}

TYPED_TEST(ProjectileTest, AccelerationCanAffectEveryAxis) {
  const auto result =
      pdef::update_balistic(this->initial, TypeParam{2, -4, 6}, 0.5);
  expect_ballistic(result, this->initial, {2, -4, 6}, 0.5);
}

TYPED_TEST(ProjectileTest, EngineCopiesInputAndExposesDefaultGravity) {
  pdef::Projectiles<TypeParam> input{this->initial};
  psim::ProjectileEngine<TypeParam> engine(input);
  EXPECT_EQ(engine.gravity(),
            static_cast<typename TestFixture::Scalar>(psim::default_gravity));
  input[0].position = TypeParam{100};
  engine.tick(0.5);
  ASSERT_EQ(engine.projectiles().size(), 1u);
  expect_ballistic(engine.projectiles()[0], this->initial,
                   {0, -double(engine.gravity()), 0}, 0.5);
  EXPECT_EQ(input[0].position, TypeParam{100});
  EXPECT_EQ(input[0].lifetime, 10);
}

TYPED_TEST(ProjectileTest, EmptyEngineCanTick) {
  pdef::Projectiles<TypeParam> input;
  psim::ProjectileEngine<TypeParam> engine(input);
  engine.tick(1);
  EXPECT_TRUE(engine.projectiles().empty());
}

TYPED_TEST(ProjectileTest, ZeroTickLeavesEngineStateUnchanged) {
  pdef::Projectiles<TypeParam> input{this->initial};
  psim::ProjectileEngine<TypeParam> engine(input);
  engine.tick(0);
  ASSERT_EQ(engine.projectiles().size(), input.size());
  expect_same(engine.projectiles()[0], input[0]);
}

TYPED_TEST(ProjectileTest, CustomZeroGravityProducesLinearMotion) {
  pdef::Projectiles<TypeParam> input{this->initial};
  psim::ProjectileEngine<TypeParam> engine(input, 0);
  engine.tick(2);
  EXPECT_EQ(engine.gravity(), 0);
  ASSERT_EQ(engine.projectiles().size(), 1u);
  expect_same(engine.projectiles()[0],
              typename TestFixture::Projectile{{9, 12, 15}, {4, 5, 6}, 8, 42});
}

TYPED_TEST(ProjectileTest, CustomGravityIsUsedOnEveryTick) {
  pdef::Projectiles<TypeParam> input{this->initial};
  psim::ProjectileEngine<TypeParam> engine(input, 10);
  engine.tick(0.5);
  engine.tick(1.5);
  ASSERT_EQ(engine.projectiles().size(), 1u);
  expect_ballistic(engine.projectiles()[0], this->initial, {0, -10, 0}, 2);
}

TYPED_TEST(ProjectileTest, ExpiredEntriesStayFrozenAndRetainOrder) {
  pdef::Projectiles<TypeParam> input{{{1, 2, 3}, {4, 5, 6}, 0, 7},
                                     {{7, 8, 9}, {1, 2, 3}, -1, 7},
                                     this->initial};
  psim::ProjectileEngine<TypeParam> engine(input, 10);
  engine.tick(1);
  engine.tick(1);
  ASSERT_EQ(engine.projectiles().size(), input.size());
  expect_same(engine.projectiles()[0], input[0]);
  expect_same(engine.projectiles()[1], input[1]);
  expect_ballistic(engine.projectiles()[2], input[2], {0, -10, 0}, 2);
}

TYPED_TEST(ProjectileTest, ExactExpiryAdvancesThenFreezes) {
  this->initial.lifetime = 1;
  pdef::Projectiles<TypeParam> input{this->initial};
  psim::ProjectileEngine<TypeParam> engine(input, 10);
  engine.tick(1);
  ASSERT_EQ(engine.projectiles().size(), 1u);
  expect_ballistic(engine.projectiles()[0], input[0], {0, -10, 0}, 1);
  const auto expired = engine.projectiles()[0];
  EXPECT_EQ(expired.lifetime, 0);
  engine.tick(10);
  expect_same(engine.projectiles()[0], expired);
}

TYPED_TEST(ProjectileTest, MidTickExpiryAdvancesFullTickThenFreezes) {
  this->initial.lifetime = 0.25;
  pdef::Projectiles<TypeParam> input{this->initial};
  psim::ProjectileEngine<TypeParam> engine(input, 10);
  engine.tick(1);
  ASSERT_EQ(engine.projectiles().size(), 1u);
  expect_ballistic(engine.projectiles()[0], input[0], {0, -10, 0}, 1);
  const auto expired = engine.projectiles()[0];
  EXPECT_EQ(expired.lifetime, -0.75);
  engine.tick(1);
  expect_same(engine.projectiles()[0], expired);
}

struct AccuracyRun {
  int rate;
  int seconds;
};
class AccuracyTest : public testing::TestWithParam<AccuracyRun> {};

template <pdef::ProjectileVector Vec> void check_accuracy(AccuracyRun run) {
  using Scalar = typename Vec::value_type;
  constexpr bool single = std::is_same_v<Scalar, float>;
  std::mt19937 generator(5000);
  std::uniform_real_distribution<double> positions(-30, 30),
      velocities(-10, 10);
  pdef::Projectiles<Vec> initial;
  for (int i = 0; i < 64; ++i) {
    initial.push_back(
        {Vec{positions(generator), positions(generator), positions(generator)},
         Vec{velocities(generator), velocities(generator),
             velocities(generator)},
         Scalar(run.seconds + 10), static_cast<pdef::ProjectileId>(i % 3)});
  }
  psim::ProjectileEngine<Vec> engine(initial);
  const Scalar dt = Scalar{1} / Scalar(run.rate);
  const int ticks = run.rate * run.seconds;
  for (int i = 0; i < ticks; ++i)
    engine.tick(dt);
  // Check size first: a zipped/prefix comparison could hide missing entries.
  ASSERT_EQ(engine.projectiles().size(), initial.size());
  const double time = double(ticks) * double(dt);
  double max_position_error = 0, max_velocity_error = 0, max_lifetime_error = 0;
  for (std::size_t i = 0; i < initial.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "projectile=" << i << " rate="
                                    << run.rate << " duration=" << run.seconds);
    expect_ballistic(
        engine.projectiles()[i], initial[i], {0, -double(engine.gravity()), 0},
        // Float motion budget: 1cm + 0.02% through 60s;
        // 2cm + 0.1% for the ten-minute accumulation stress test.
        time, single ? (run.seconds > 60 ? 0.02 : 0.01) : 1e-8,
        single ? (run.seconds > 60 ? 1e-3 : 2e-4) : 1e-10,
        // Repeated float subtraction loses precision while lifetime
        // is large. Budget 0.02% of the starting lifetime separately
        // from the motion tolerances (0.122s for a 610s lifetime).
        single ? 0.0002 * double(initial[i].lifetime) : 1e-8);
    for (int axis = 0; axis < 3; ++axis) {
      const double acceleration = axis == 1 ? -double(engine.gravity()) : 0;
      const double position = double(initial[i].position[axis]) +
                              double(initial[i].velocity[axis]) * time +
                              0.5 * acceleration * time * time;
      const double velocity =
          double(initial[i].velocity[axis]) + acceleration * time;
      max_position_error = std::max(
          max_position_error,
          std::abs(double(engine.projectiles()[i].position[axis]) - position));
      max_velocity_error = std::max(
          max_velocity_error,
          std::abs(double(engine.projectiles()[i].velocity[axis]) - velocity));
    }
    max_lifetime_error = std::max(
        max_lifetime_error, std::abs(double(engine.projectiles()[i].lifetime) -
                                     (double(initial[i].lifetime) - time)));
  }
  testing::Test::RecordProperty("max_position_error",
                                std::format("{:.17g}", max_position_error));
  testing::Test::RecordProperty("max_velocity_error",
                                std::format("{:.17g}", max_velocity_error));
  testing::Test::RecordProperty("max_lifetime_error",
                                std::format("{:.17g}", max_lifetime_error));
}

TEST_P(AccuracyTest, FloatMatchesIndependentDoubleReference) {
  check_accuracy<glm::vec3>(GetParam());
}
TEST_P(AccuracyTest, DoubleMatchesIndependentDoubleReference) {
  check_accuracy<glm::dvec3>(GetParam());
}
INSTANTIATE_TEST_SUITE_P(
    TickRatesAndDurations, AccuracyTest,
    testing::Values(AccuracyRun{30, 60}, AccuracyRun{60, 60},
                    AccuracyRun{120, 60}, AccuracyRun{60, 1},
                    AccuracyRun{60, 10}, AccuracyRun{60, 600}),
    [](const testing::TestParamInfo<AccuracyRun> &info) {
      return "Hz" + std::to_string(info.param.rate) + "Seconds" +
             std::to_string(info.param.seconds);
    });
} // namespace
