#include "drake/multibody/rigid_body_plant/kinematics_results.h"

#include "drake/common/default_scalars.h"
#include "drake/common/eigen_types.h"
#include "drake/multibody/kinematics_cache.h"
#include "drake/util/drakeGeometryUtil.h"

namespace drake {
namespace systems {

template<typename T>
KinematicsResults<T>::KinematicsResults(const RigidBodyTree<double>* tree) :
    tree_(tree), kinematics_cache_(tree_->CreateKinematicsCacheWithType<T>()) {
}

template <typename T>
void KinematicsResults<T>::Update(const Eigen::Ref<const VectorX<T>>& q,
                                  const Eigen::Ref<const VectorX<T>>& v) {
  this->kinematics_cache_.initialize(q, v);
  this->tree_->doKinematics(this->kinematics_cache_, false);
}

template <typename T>
int KinematicsResults<T>::get_num_bodies() const {
  return tree_->get_num_bodies();
}

template<typename T>
int KinematicsResults<T>::get_num_positions() const {
  return kinematics_cache_.get_num_positions();
}

template<typename T>
int KinematicsResults<T>::get_num_velocities() const {
  return kinematics_cache_.get_num_velocities();
}

template<typename T>
Quaternion<T> KinematicsResults<T>::get_body_orientation(int body_index) const {
  Isometry3<T>
      pose = tree_->relativeTransform(kinematics_cache_, 0, body_index);
  const drake::math::RotationMatrix<T> R(pose.linear());
  return R.ToQuaternion();
}

template<typename T>
Vector3<T> KinematicsResults<T>::get_body_position(int body_index) const {
  Isometry3<T>
      pose = tree_->relativeTransform(kinematics_cache_, 0, body_index);
  return pose.translation();
}

template <typename T>
Isometry3<T> KinematicsResults<T>::get_pose_in_world(
    const RigidBody<T>& body) const {
  const auto& world = tree_->world();
  return tree_->relativeTransform(kinematics_cache_, world.get_body_index(),
                                  body.get_body_index());
}

template <typename T>
TwistVector<T> KinematicsResults<T>::get_twist_in_world_frame(
    const RigidBody<T>& body) const {
  const auto& world = tree_->world();
  return tree_->relativeTwist(kinematics_cache_, world.get_body_index(),
                              body.get_body_index(), 0);
}

template <typename T>
TwistVector<T> KinematicsResults<T>::get_twist_in_world_aligned_body_frame(
    const RigidBody<T>& body) const {
  auto twist_in_world = get_twist_in_world_frame(body);
  auto body_to_world = get_pose_in_world(body);
  Isometry3<T> world_to_world_aligned_body(Isometry3<T>::Identity());
  world_to_world_aligned_body.translation() = -body_to_world.translation();
  return transformSpatialMotion(world_to_world_aligned_body, twist_in_world);
}

template <typename T>
void KinematicsResults<T>::UpdateFromContext(const Context<T>& context) {
  const int nq = tree_->get_num_positions();
  const int nv = tree_->get_num_velocities();

  VectorX<T> x;
  if (context.get_state().get_continuous_state().size() > 0) {
    // TODO(amcastro-tri): provide nicer accessor to an Eigen representation for
    // LeafSystems.
    x = dynamic_cast<const BasicVector<T>&>(
        context.get_continuous_state_vector()).get_value();
  } else {
    x = dynamic_cast<const BasicVector<T>&>(
        context.get_discrete_state(0)).get_value();
  }

  const auto q = x.topRows(nq);
  const auto v = x.bottomRows(nv);
  kinematics_cache_.initialize(q, v);
  tree_->doKinematics(kinematics_cache_, false);
}

template <typename T>
Eigen::VectorBlock<const VectorX<T>> KinematicsResults<T>::get_joint_position(
    const RigidBody<T>& body) const {
  return kinematics_cache_.getQ().segment(body.get_position_start_index(),
                                          body.getJoint().get_num_positions());
}

template <typename T>
Eigen::VectorBlock<const VectorX<T>> KinematicsResults<T>::get_joint_velocity(
    const RigidBody<T>& body) const {
  return kinematics_cache_.getV().segment(body.get_velocity_start_index(),
                                          body.getJoint().get_num_velocities());
}

}  // namespace systems
}  // namespace drake

DRAKE_DEFINE_CLASS_TEMPLATE_INSTANTIATIONS_ON_DEFAULT_NONSYMBOLIC_SCALARS(
    class ::drake::systems::KinematicsResults)
