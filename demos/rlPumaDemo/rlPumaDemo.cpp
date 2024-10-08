//
// Copyright (c) 2009, Markus Rickert
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <boost/lexical_cast.hpp>
#include <rl/kin/Kinematics.h>
#include <rl/kin/Puma.h>
#include <rl/kin/XmlFactory.h>
#include <rl/math/Constants.h>
#include <rl/math/Rotation.h>

int
main(int argc, char** argv)
{
	if (argc < 8)
	{
		std::cout << "Usage: rlPumaDemo PUMAFILE Q1 Q2 Q3 Q4 Q5 Q6" << std::endl;
		return EXIT_FAILURE;
	}
	
	try
	{
		rl::kin::XmlFactory factory;
		std::shared_ptr<rl::kin::Kinematics> kinematics(factory.create(argv[1]));
		
		if (rl::kin::Puma* puma = dynamic_cast<rl::kin::Puma*>(kinematics.get()))
		{
			rl::math::Vector q(puma->getDof());
			
			for (std::ptrdiff_t i = 0; i < q.size(); ++i)
			{
				q(i) = boost::lexical_cast<rl::math::Real>(argv[i + 2]) * rl::math::constants::deg2rad;
			}
			
			rl::kin::Puma::Arm arm;
			rl::kin::Puma::Elbow elbow;
			rl::kin::Puma::Wrist wrist;
			
			puma->parameters(q, arm, elbow, wrist);
			
			puma->setArm(arm);
			puma->setElbow(elbow);
			puma->setWrist(wrist);
			
			std::cout
				<< "Arm: " << (arm == rl::kin::Puma::Arm::left ? "LEFT" : "RIGHT")
				<< ", Elbow: " << (elbow == rl::kin::Puma::Elbow::above ? "ABOVE" : "BELOW")
				<< ", Wrist: " << (wrist == rl::kin::Puma::Wrist::flip ? "FLIP" : "NONFLIP")
				<< std::endl;
			
			std::cout << "q: " << q.transpose() * rl::math::constants::rad2deg << std::endl;
			
			puma->setPosition(q);
			puma->updateFrames();
			
			rl::math::Vector3 position = puma->forwardPosition().translation();
			rl::math::Vector3 orientation = puma->forwardPosition().rotation().eulerAngles(2, 1, 0).reverse();
			
			std::cout << "x: " << position.x() << " m, y: " << position.y() << " m, z: " << position.z() << " m, a: " << orientation.x() * rl::math::constants::rad2deg << " deg, b: " << orientation.y() * rl::math::constants::rad2deg << " deg, c: " << orientation.z() * rl::math::constants::rad2deg << " deg" << std::endl;
			
			rl::math::Vector q2(puma->getDof());
			q2.setConstant(1.0 * rl::math::constants::deg2rad);
			
			rl::math::Transform x = puma->forwardPosition();
			
			if (!puma->inversePosition(x, q2))
			{
				std::cout << "out of reach" << std::endl;
				return EXIT_FAILURE;
			}
			
			std::cout << "q: " << q2.transpose() * rl::math::constants::rad2deg << std::endl;
			
			puma->setPosition(q2);
			puma->updateFrames();
			
			position = puma->forwardPosition().translation();
			orientation = puma->forwardPosition().rotation().eulerAngles(2, 1, 0).reverse();
			
			std::cout << "x: " << position.x() << " m, y: " << position.y() << " m, z: " << position.z() << " m, a: " << orientation.x() * rl::math::constants::rad2deg << " deg, b: " << orientation.y() * rl::math::constants::rad2deg << " deg, c: " << orientation.z() * rl::math::constants::rad2deg << " deg" << std::endl;
		}
		else
		{
			return EXIT_FAILURE;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
