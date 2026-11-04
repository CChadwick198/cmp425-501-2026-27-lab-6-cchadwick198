#pragma once

#include <SFML\Network.hpp>
#include <deque>

enum PredictionMethod { None, Linear, Quadratic, Cubic };
enum ConnState {NotReady, Ready, Disconnected};

constexpr PredictionMethod PREDICTION_METHOD_IN_USE = Linear;

// Fixme:: ideally we should link specific tank/character to each connection,
// rather than assuming the order in which connections are made is the same as
// the order in which tanks/characters are stored in the game.

// This could also be made into a class, given the increased functionality.
class Connection
{
public:	// For now, just have everything public cause I don't want to sort out the conflicts.

	std::unique_ptr<sf::TcpSocket> socket;
	ConnState state = Disconnected;

	std::deque<TankMessage> message_history;
	float inter_t = 0.f; // This value goes from 0.f to 1.f during interpolation and will be accumulated based on time delta.

	// Interpolate position based on selected method and value of t. Return TankMessage with updated position.
	sf::Vector2f Interpolate(float dt) {
		// Calculate interpolation step
		float step = 0.f;
		
		if(message_history.size() > 1) // Make sure we have at least 2 messages for Linear interpolation.
			step = 1.f / ((message_history[0].time - message_history[1].time) / dt);
		else
			step = 0.f;

		// Add step to inter_t
		inter_t += step;

		// Clamp inter_t to max of 1.f
		if(inter_t > 1.f)
			inter_t = 1.f;

		if (message_history.size() > 1) { // Make sure we have at least 2 messages for Linear interpolation.
			// Calculate new position by interpolating between last two known positons.
			//return (message_history[1].position + (message_history[0].position - message_history[1].position) * inter_t);

			// Remember that message_history[0].position is the latest position (v1).
			return (1 - inter_t) * message_history[1].position + inter_t * message_history[0].position;
		}
		else
			return message_history[0].position; // Just return the last known position.
	}

	sf::Vector2f InterpolateToPredicted(float dt, sf::Vector2f predicted_position) {
		// Calculate interpolation step
		float step = 0.f;

		// pp_time - last_message_time = prediction_time + last_message_time - last_message_time

		if (!message_history.empty()) // Make sure we have at least 2 messages for Linear interpolation.
			step = 1.f / (prediction_time / dt);
		else
			step = 0.f;

		// Add step to inter_t
		inter_t += step;

		// Clamp inter_t to max of 1.f
		if (inter_t > 1.f)
			inter_t = 1.f;

		if (!message_history.empty()) { // Make sure we have at least 2 messages for Linear interpolation.
			// Remember that message_history[0].position is the latest position (v1).
			return (1 - inter_t) * message_history[0].position + inter_t * predicted_position;
		}
		else
		{
			return predicted_position; // Just return the predicted position.
		}
	}

	sf::Vector2f Predict(float t, float dt) {
		// Get the time difference between the current game time and the time of the last recieved message.
		prediction_time = t - message_history[0].time;

		// Predict the tank position differently depending on the prediction method in use.
		switch (PREDICTION_METHOD_IN_USE)
		{
		case None:
			return message_history[0].position; // Just return the last known position.
			break;
		case Linear:
			return LinearPrediction(dt);
			break;
		case Quadratic:
			return QuadraticPrediction(dt);
			break;
		case Cubic:
			return CubicPrediction(dt);
			break;
		}
	}

//protected:

	float prediction_time = 0.f;

	sf::Vector2f LinearPrediction(float dt) {
		// Check to see if we have enough known positions to do prediction.
		if (message_history.size() < 2) { return message_history[0].position; }	// Just return the last known position.

		sf::Vector2f velocity = (message_history[0].position - message_history[1].position) / dt;

		return message_history[0].position + velocity * prediction_time;
	}

	sf::Vector2f QuadraticPrediction(float dt) {
		// Check to see if we have enough known positions to do prediction.
		if (message_history.size() < 3) { return message_history[0].position; }	// Just return the last known position.

		sf::Vector2f velocity1 = (message_history[1].position - message_history[2].position) / dt;
		sf::Vector2f velocity2 = (message_history[0].position - message_history[1].position) / dt;

		sf::Vector2f acc = (velocity2 - velocity1) / dt;

		// predicted position = p0 + v2 * pt + 0.5 * A * pt^2
		sf::Vector2f predicted_position = message_history[0].position + velocity2 * prediction_time + 0.5f * acc * prediction_time * prediction_time;

		return predicted_position;
	}

	sf::Vector2f CubicPrediction(float dt) {
		return message_history[0].position;
	}
};