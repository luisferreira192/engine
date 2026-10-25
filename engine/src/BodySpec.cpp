#include "engine/BodySpec.h"

Body BodySpec::build(float default_restitution) const
{
    Body body(position_, velocity_, collider_);

    body.restitution = restitution_.value_or(default_restitution);
    body.locked = locked_;

    // A locked axis never carries velocity.
    if (locked_.x) body.velocity.x = 0.0f;
    if (locked_.y) body.velocity.y = 0.0f;

    if (fixed_)
        body.make_static();
    else
        body.set_mass(mass_);

    return body;
}
